#include "display/ui.hpp"

namespace kc {

pt::time_duration Display::Ui::TimeToNextMinute()
{
    pt::ptime now = pt::microsec_clock::local_time();
    pt::ptime nextMinute = now + pt::minutes(1);
    return pt::ptime(nextMinute.date(), pt::time_duration(nextMinute.time_of_day().hours(), nextMinute.time_of_day().minutes(), 0)) - now;
}

void Display::Ui::updateNextEventInfo(pt::ptime now)
{
    if (!m_nextEvent)
    {
        print(0, 11, "\3\3\6\3\3");
        return;
    }

    size_t secondsTo = std::abs((m_nextEvent->timestamp() - now).total_seconds());
    if (secondsTo > 99 * 60)
        secondsTo = 99 * 60;

    print(0, 11, fmt::format(
        "{}\6{:0>2}",
        m_nextEvent->shortName(),
        std::round(secondsTo / 60.0)
    ));
}

void Display::Ui::updateFunction()
{
    while (true)
    {
        std::unique_lock lock(m_mutex);
        if (m_updateThreadStatus == ThreadStatus::Stopped)
            return;
        lock.unlock();

        {
            std::lock_guard updateLock(m_updateMutex);
            if (m_updateThreadStatus == ThreadStatus::Stopped)
                return;

            pt::ptime now = pt::second_clock::local_time() + pt::seconds(10);
            updateNextEventInfo(now);
            print(1, 11, fmt::format(
                "{:0>2}:{:0>2}",
                now.time_of_day().hours(),
                now.time_of_day().minutes()
            ));

            try
            {
                Sensors::Measurement externalMeasurement = Sensors::Measure(Sensors::Location::External);
                externalMeasurement.aht20.humidity = Utility::Limit(externalMeasurement.aht20.humidity, 0, 99.9);
                print(0, 0, fmt::format(
                    "{:>5.1f}|{:4.1f}|",
                    externalMeasurement.bmp280.temperature,
                    externalMeasurement.aht20.humidity
                ));
            }
            catch (...)
            {
                print(0, 0, "   FAIL   |");
            }

            try
            {
                Sensors::Measurement internalMeasurement = Sensors::Measure(Sensors::Location::Internal);
                internalMeasurement.aht20.humidity = Utility::Limit(internalMeasurement.aht20.humidity, 0, 99.9);
                print(1, 0, fmt::format(
                    "{:>5.1f}|{:4.1f}|",
                    internalMeasurement.bmp280.temperature,
                    internalMeasurement.aht20.humidity
                ));
            }
            catch (...)
            {
                print(1, 0, "   FAIL   |");
            }
        }

        lock.lock();
        if (m_updateThreadStatus == ThreadStatus::Stopped)
            return;
        if (Utility::InterSleep(lock, m_cv, TimeToNextMinute().total_milliseconds() / 1000.0))
            return;
    }
}

void Display::Ui::messageFunction()
{
    {
        std::lock_guard lock(m_mutex);
        if (m_queue.empty())
        {
            m_messageThreadStatus = ThreadStatus::Idle;
            return;
        }
        m_messageThreadStatus = ThreadStatus::Running;
    }

    std::lock_guard updateLock(m_updateMutex);
    Master::Screen previousScreen = screen();
    while (true)
    {
        Message message;
        {
            std::lock_guard lock(m_mutex);
            if (m_queue.empty())
                break;
            message = std::move(m_queue[0]);
            m_queue.pop_front();
        }

        backlight(false);
        clear();
        for (size_t index = 0, size = message.size(); index < size; ++index)
        {
            if (!backlight())
            {
                Utility::Sleep(0.3);
                std::lock_guard lock(m_mutex);
                if (m_messageThreadStatus == ThreadStatus::Stopped)
                    return;
                backlight(true);
            }

            const Screen& screen = message[index];
            if (!screen.blinks)
            {
                std::unique_lock lock(m_mutex);
                print(0, 0, screen.line1);
                print(1, 0, screen.line2);
                if (m_messageThreadStatus == ThreadStatus::Stopped)
                    return;
                if (Utility::InterSleep(lock, m_cv, screen.delay))
                    return;
            }
            else
            {
                for (uint32_t blink = 0; blink < screen.blinks; ++blink)
                {
                    std::unique_lock lock(m_mutex);
                    print(0, 0, screen.line1);
                    print(1, 0, screen.line2);
                    if (m_messageThreadStatus == ThreadStatus::Stopped)
                        return;
                    if (Utility::InterSleep(lock, m_cv, screen.delay))
                        return;

                    if (blink + 1 != screen.blinks)
                    {
                        clear();
                        if (m_messageThreadStatus == ThreadStatus::Stopped)
                            return;
                        if (Utility::InterSleep(lock, m_cv, screen.delay))
                            return;
                    }
                }
            }
            
            if (index + 1 != size)
            {
                clear();
                Utility::Sleep(0.3);
            }
        }
    }

    backlight(false);
    print(previousScreen);
    updateNextEventInfo(pt::second_clock::local_time());
    Utility::Sleep(0.3);

    std::lock_guard lock(m_mutex);
    if (m_messageThreadStatus != ThreadStatus::Stopped)
    {
        backlight(true);
        m_messageThreadStatus = ThreadStatus::Idle;
    }
}

Display::Ui::Ui()
    : Master(Config::Instance->internalPort(), false)
    , m_updateThreadStatus(ThreadStatus::Idle)
    , m_messageThreadStatus(ThreadStatus::Idle)
{
    // Display UI reset
    disable();
}

Display::Ui::~Ui()
{
    // Display UI reset
    disable();
}

bool Display::Ui::enabled()
{
    std::lock_guard lock(m_mutex);
    return (m_updateThreadStatus == ThreadStatus::Running);
}

void Display::Ui::enable()
{
    std::unique_lock lock(m_mutex);
    if (m_updateThreadStatus == ThreadStatus::Running)
        return;
    m_updateThreadStatus = ThreadStatus::Running;
    lock.unlock();

    static bool startupDisplayed = false;
    if (!startupDisplayed)
    {
        configure(true, false, false);
        showMessage({
            {
                " Copaipy        ",
                " Configuration: ",
                2.0
            },
            {
                fmt::format("HTTP port {:>6}", Config::Instance->httpPort()),
                fmt::format("Time res. {:>5.1f}s", Config::Instance->timeReserve() / 1000.0)
            },
            {
                fmt::format("Latitude {:>6.1f}\337", Config::Instance->latitude()),
                fmt::format("Longitude {:>5.1f}\337", Config::Instance->longitude())
            },
            {
                fmt::format("Sunrise {:>7.3f}\337", Config::Instance->sunriseAngle()),
                fmt::format("Sunset {:>8.3f}\337", Config::Instance->sunsetAngle())
            },
        });
        startupDisplayed = true;
        Utility::Sleep(0.1);
    }
    else
    {
        configure(true, false, false);
        backlight(true);
    }

    lock.lock();
    m_updateThread = std::thread(&Ui::updateFunction, this);
}

void Display::Ui::disable()
{
    std::unique_lock lock(m_mutex);
    m_updateThreadStatus = ThreadStatus::Stopped;
    m_messageThreadStatus = ThreadStatus::Stopped;

    lock.unlock();
    m_cv.notify_all();
    if (m_updateThread.joinable())
        m_updateThread.join();
    m_updateThreadStatus = ThreadStatus::Idle;
    if (m_messageThread.joinable())
        m_messageThread.join();
    m_messageThreadStatus = ThreadStatus::Idle;
    lock.lock();

    configure(false, false, false);
    backlight(false);
    clear();
}

void Display::Ui::showMessage(const Message& message)
{
    std::lock_guard lock(m_mutex);
    if (m_updateThreadStatus != ThreadStatus::Running)
        return;
    m_queue.push_back(message);

    if (m_messageThreadStatus == ThreadStatus::Idle)
    {
        if (m_messageThread.joinable())
            m_messageThread.join();
        m_messageThreadStatus = ThreadStatus::Running;
        m_messageThread = std::thread(&Ui::messageFunction, this);
    }
}

void Display::Ui::updateNextEvent(Capture::Event* event)
{
    {
        std::lock_guard lock(m_mutex);
        if (event)
            m_nextEvent = std::make_unique<Capture::Event>(event->name(), event->shortName(), event->timestamp());
        else
            m_nextEvent.reset();
    }

    if (m_messageThreadStatus != ThreadStatus::Running)
    {
        std::lock_guard updateLock(m_updateMutex);
        updateNextEventInfo(pt::second_clock::local_time());
    }
}

} // namespace kc

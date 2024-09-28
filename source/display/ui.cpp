#include "display/ui.hpp"

namespace kc {

char Display::Ui::TrendSymbol(double trend)
{
    if (trend > 1.0)
        return '\6';
    if (trend > 0.3)
        return '\4';
    if (trend >= -0.3)
        return '-';
    if (trend >= -1.0)
        return '\5';
    return '\7';
}

void Display::Ui::updateNextEventInfo(pt::ptime now)
{
    if (!m_nextEvent)
    {
        print(0, 11, "\3\3\3\3\3");
        return;
    }

    size_t secondsTo = std::abs((m_nextEvent->timestamp() - now).total_seconds());
    if (secondsTo > 99 * 60)
        secondsTo = 99 * 60;

    print(0, 11, fmt::format(
        "{}\3{:0>2}",
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

            Sensors::Recorder::Record record = Sensors::Recorder::Instance->last();
            Sensors::Recorder::Record trend = Sensors::Recorder::Instance->trend();

            if (record.external)
            {
                record.external->aht20.humidity = Utility::Limit(record.external->aht20.humidity, 0, 99.9);
                print(0, 0, fmt::format(
                    "{:>5.1f}{}{:4.1f}{}",
                    record.external->bmp280.temperature,
                    trend.external ? TrendSymbol(trend.external->bmp280.temperature) : '\3',
                    record.external->aht20.humidity,
                    trend.external ? TrendSymbol(trend.external->aht20.humidity) : '\3'
                ));
            }
            else
            {
                print(0, 0, "   FAIL   |");
            }

            if (record.internal)
            {
                record.internal->aht20.humidity = Utility::Limit(record.internal->aht20.humidity, 0, 99.9);
                print(1, 0, fmt::format(
                    "{:>5.1f}{}{:4.1f}{}",
                    record.internal->bmp280.temperature,
                    trend.internal ? TrendSymbol(trend.internal->bmp280.temperature) : '\3',
                    record.internal->aht20.humidity,
                    trend.internal ? TrendSymbol(trend.internal->aht20.humidity) : '\3'
                ));
            }
            else
            {
                print(1, 0, "   FAIL   |");
            }
        }

        lock.lock();
        if (m_updateThreadStatus == ThreadStatus::Stopped)
            return;
        if (Utility::InterSleep(lock, m_cv, Utility::TimeToNextMinute().total_milliseconds() / 1000.0))
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

            if (message.empty())
            {
                m_messageThreadStatus = ThreadStatus::Idle;
                return;
            }
        }

        backlight(false);
        clear();
        print(0, 0, message[0].line1);
        print(1, 0, message[0].line2);
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

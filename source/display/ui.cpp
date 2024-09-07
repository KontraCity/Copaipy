#include "display/ui.hpp"

namespace kc {

pt::time_duration Display::Ui::TimeToNextMinute()
{
    pt::ptime now = pt::microsec_clock::local_time();
    pt::ptime nextMinute = now + pt::minutes(1);
    return pt::ptime(nextMinute.date(), pt::time_duration(nextMinute.time_of_day().hours(), nextMinute.time_of_day().minutes(), 0)) - now;
}

void Display::Ui::updateEventTimestampDuration(pt::ptime now)
{
    pt::time_duration duration = now - m_eventTimestamp;
    if (m_eventTimestamp.is_not_a_date_time() || std::abs(duration.total_seconds()) > 100 * 60 * 60) // We can only display up to "99:59"
    {
        print(0, 11, "\3\3:\3\3");
        return;
    }

    print(0, 11, fmt::format(
        "{:0>2}:{:0>2}",
        std::abs(duration.hours()),
        std::abs(duration.minutes())
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
            updateEventTimestampDuration(now);
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
                    "{: >5.1f}|{: >4.1f}|",
                    externalMeasurement.bmp280.temperature,
                    externalMeasurement.aht20.humidity
                ));

                Sensors::Measurement internalMeasurement = Sensors::Measure(Sensors::Location::Internal);
                internalMeasurement.aht20.humidity = Utility::Limit(internalMeasurement.aht20.humidity, 0, 99.9);
                print(1, 0, fmt::format(
                    "{: >5.1f}|{: >4.1f}|",
                    internalMeasurement.bmp280.temperature,
                    internalMeasurement.aht20.humidity
                ));
            }
            catch (...)
            {
                print(0, 0, fmt::format("{: <16}", "Measurement"));
                print(1, 0, fmt::format(
                    "{: <11}",
                    "Failure \2"
                ));
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
        if (message.submessages.empty())
            message.submessages.push_back({ "[no submessages]" });

        backlight(false);
        clear();
        print(0, 0, message.header);
        for (size_t index = 0, size = message.submessages.size(); index < size; ++index)
        {
            if (!backlight())
            {
                Utility::Sleep(0.3);
                std::lock_guard lock(m_mutex);
                if (m_messageThreadStatus == ThreadStatus::Stopped)
                    return;
                backlight(true);
            }

            const Message::Submessage& submessage = message.submessages[index];
            if (!submessage.blinks)
            {
                std::unique_lock lock(m_mutex);
                print(1, 0, submessage.string);
                if (m_messageThreadStatus == ThreadStatus::Stopped)
                    return;
                if (Utility::InterSleep(lock, m_cv, submessage.delay))
                    return;
            }
            else
            {
                for (uint32_t blink = 0; blink < submessage.blinks; ++blink)
                {
                    std::unique_lock lock(m_mutex);
                    print(1, 0, submessage.string);
                    if (m_messageThreadStatus == ThreadStatus::Stopped)
                        return;
                    if (Utility::InterSleep(lock, m_cv, submessage.delay))
                        return;

                    if (blink + 1 != submessage.blinks)
                    {
                        print(1, 0, std::string(16, ' '));
                        if (m_messageThreadStatus == ThreadStatus::Stopped)
                            return;
                        if (Utility::InterSleep(lock, m_cv, submessage.delay))
                            return;
                    }
                }
            }
            
            if (index + 1 != size)
            {
                print(1, 0, std::string(16, ' '));
                Utility::Sleep(0.3);
            }
        }
    }

    backlight(false);
    print(previousScreen);
    updateEventTimestampDuration(pt::second_clock::local_time());
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
        showMessage({ fmt::format("{:^16}", "[Startup]"), {
            { fmt::format("HTTP port:{:>6}", Config::Instance->httpPort()), 2.0 },
            { fmt::format("Time res.:{:>5.2f}s", Config::Instance->timeReserve() / 1000.0), 2.0 },
            { fmt::format("Latitude:{:>6.1f}\337", Config::Instance->latitude()), 2.0 },
            { fmt::format("Longitude:{:>5.1f}\337", Config::Instance->longitude()), 2.0 },
            { fmt::format("Sunrise:{:>7.3f}\337", Config::Instance->sunriseAngle()), 2.0 },
            { fmt::format("Sunset:{:>8.3f}\337", Config::Instance->sunsetAngle()), 2.0 }
        }});
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

void Display::Ui::updateEventTimestamp(pt::ptime timestamp)
{
    std::lock_guard lock(m_mutex);
    m_eventTimestamp = timestamp;
    if (m_messageThreadStatus != ThreadStatus::Running)
        updateEventTimestampDuration(pt::second_clock::local_time());
}

} // namespace kc

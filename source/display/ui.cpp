#include "display/ui.hpp"

namespace kc {

pt::time_duration Display::Ui::TimeToNextMinute()
{
    pt::ptime now = pt::microsec_clock::local_time();
    pt::ptime nextMinute = now + pt::minutes(1);
    return pt::ptime(nextMinute.date(), pt::time_duration(nextMinute.time_of_day().hours(), nextMinute.time_of_day().minutes(), 0)) - now;
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
            pt::ptime now = pt::second_clock::local_time() + pt::seconds(10);
            print(1, 11, fmt::format(
                "{:0>2}:{:0>2}",
                now.time_of_day().hours(),
                now.time_of_day().minutes()
            ));

            try
            {
                Sensors::Measurement externalMeasurement = Sensors::Measure(m_config, Sensors::Location::External);
                externalMeasurement.aht20.humidity = Utility::Limit(externalMeasurement.aht20.humidity, 0, 99.9);
                print(0, 0, fmt::format(
                    "{: >5.1f}|{: >4.1f}|",
                    externalMeasurement.bmp280.temperature,
                    externalMeasurement.aht20.humidity
                ));

                Sensors::Measurement internalMeasurement = Sensors::Measure(m_config, Sensors::Location::Internal);
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
            message.submessages.push_back("[no submessages]");

        backlight(false);
        clear();
        print(0, 0, message.header);
        for (const std::string& submessage : message.submessages)
        {
            if (!backlight())
            {
                Utility::Sleep(0.3);
                std::lock_guard lock(m_mutex);
                if (m_messageThreadStatus == ThreadStatus::Stopped)
                    return;
                backlight(true);
            }

            position(1, 0);
            for (char character : fmt::format("{: <16}", submessage))
            {
                print(character);
                Utility::Sleep(0.03);
            }

            std::unique_lock lock(m_mutex);
            if (m_messageThreadStatus == ThreadStatus::Stopped)
                return;
            if (Utility::InterSleep(lock, m_cv, message.delayInSeconds))
                return;
        }
    }

    backlight(false);
    print(previousScreen);
    Utility::Sleep(0.3);

    std::lock_guard lock(m_mutex);
    if (m_messageThreadStatus != ThreadStatus::Stopped)
    {
        backlight(true);
        m_messageThreadStatus = ThreadStatus::Idle;
    }
}

Display::Ui::Ui(Config::Pointer config)
    : Master(config->internalPort(), false)
    , m_config(config)
    , m_updateThreadStatus(ThreadStatus::Idle)
    , m_messageThreadStatus(ThreadStatus::Idle)
{
    // Display UI reset
    stop();
}

Display::Ui::~Ui()
{
    // Display UI reset
    stop();
}

void Display::Ui::start()
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
            fmt::format("HTTP port:{:>6}", m_config->httpPort()),
            fmt::format("Time res.:{:>5.2f}s", m_config->timeReserve() / 1000.0),
            fmt::format("Latitude:{:>6.1f}\337", m_config->latitude()),
            fmt::format("Longitude:{:>5.1f}\337", m_config->longitude()),
            fmt::format("Sunrise:{:>7.3f}\337", m_config->sunriseAngle()),
            fmt::format("Sunset:{:>8.3f}\337", m_config->sunsetAngle())
        }, 2 });
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

void Display::Ui::stop()
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

} // namespace kc

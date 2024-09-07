#include "capture/master.hpp"
using namespace kc::Capture::MasterConst;

namespace kc {

Capture::Event::Pointer Capture::Master::CreateCaptureFilesystem()
{
    if (!std::filesystem::create_directory(CaptureDirectory))
    {
        throw std::runtime_error(fmt::format(
            "kc::Capture::Master::CreateCaptureFilesystem(): "
            "Couldn't create capture directory \"{}/\"",
            CaptureDirectory
        ));
    }

    for (const std::string& task : Event::GetTasks())
    {
        std::string eventDirectory = fmt::format("{}/{}", CaptureDirectory, task);
        if (!std::filesystem::create_directory(eventDirectory))
        {
            throw std::runtime_error(fmt::format(
                "kc::Capture::Master::CreateCaptureFilesystem(): "
                "Couldn't create event directory \"{}/\"",
                eventDirectory
            ));
        }
    }

    Event::Pointer lastEvent = std::make_unique<Event>("Start", pt::second_clock::local_time());
    lastEvent->save(fmt::format("{}/{}", CaptureDirectory, LastEventFile));
    return lastEvent;
}

size_t Capture::Master::CountOverlappingEvents(const Event::Pointer& event)
{
    size_t overlappingEvents = 0;
    for (Event* overlapping = event->overlapping().get(); overlapping; overlapping = overlapping->overlapping().get())
        ++overlappingEvents;
    return overlappingEvents;
}

void Capture::Master::LogGenerationResult(const GenerationResult& result)
{
    if (!result.expired && !result.mapped)
    {
        m_logger.info(
            "Generated {} event{} for [{}]",
            result.generated, result.generated == 1 ? "" : "s", Utility::ToString(result.date)
        );
    }
    else if (result.expired && !result.mapped)
    {
        m_logger.warn(
            "Generated {} event{} for [{}], but {} {} expired! Only {} {} added to the queue",
            result.generated, result.generated == 1 ? "" : "s", Utility::ToString(result.date),
            result.expired, result.expired == 1 ? "is" : "are",
            result.generated - result.expired, result.generated - result.expired == 1 ? "was" : "were"
        );
    }
    else if (!result.expired && result.mapped)
    {
        m_logger.info(
            "Generated {} event{} for [{}], {} overlapping event{} mapped",
            result.generated, result.generated == 1 ? "" : "s", Utility::ToString(result.date),
            result.mapped, result.mapped == 1 ? " is" : "s are"
        );
    }
    else
    {
        m_logger.warn(
            "Generated {} event{} for [{}], {} overlapping event{} mapped and {} {} expired! Only {} {} added to the queue",
            result.generated, result.generated == 1 ? "" : "s", Utility::ToString(result.date),
            result.mapped, result.mapped == 1 ? " is" : "s are",
            result.expired, result.expired == 1 ? "is" : "are",
            result.generated - result.expired, result.generated - result.expired == 1 ? "was" : "were"
        );
    }
}

void Capture::Master::captureFunction()
{
    try
    {
        if (!std::filesystem::is_directory(CaptureDirectory))
        {
            m_logger.info("Creating capture filesystem");
            m_lastEvent = CreateCaptureFilesystem();
            return;
        }

        for (const std::string& task : Event::GetTasks())
        {
            std::string eventDirectory = fmt::format("{}/{}", CaptureDirectory, task);
            if (!std::filesystem::is_directory(eventDirectory))
            {
                throw std::runtime_error(fmt::format(
                    "kc::Capture::Master::Master(): "
                    "Couldn't find event directory \"{}/\"",
                    eventDirectory
                ));
            }
        }
        m_lastEvent = std::make_unique<Event>(fmt::format("{}/{}", CaptureDirectory, LastEventFile));

        for (dt::date date = m_lastEvent->timestamp().date(), today = dt::day_clock::local_day(); date <= today; date += dt::days(1))
        {
            GenerationResult result = generateEvents(date);
            if (date == today)
            {
                LogGenerationResult(result);
            }
            else if (result.expired)
            {
                m_logger.warn(
                    "{} event{} expired for [{}]",
                    result.expired,
                    result.expired == 1 ? " is" : "s are",
                    Utility::ToString(date)
                );
            }
        }

        while (true)
        {
            while (!m_queue.empty())
            {
                Event::Pointer& event = m_queue[0];
                pt::time_duration toEvent = (event->timestamp() - pt::microsec_clock::local_time());
                if (toEvent.total_milliseconds() <= Config::Instance->timeReserve())
                {
                    m_logger.error(
                        "Event [#{} \"{}\"] is expired, can't sleep [{}]!",
                        event->id(), event->name(), Utility::ToString(toEvent)
                    );

                    capture(std::move(event), true);
                    m_queue.pop_front();
                    continue;
                }

                m_logger.info(
                    "Sleeping [{}] to next event [#{} \"{}\"]",
                    Utility::ToString(toEvent), event->id(), event->name()
                );
                {
                    std::unique_lock lock(m_mutex);
                    if (m_threadStatus == ThreadStatus::Stopped)
                        return;
                    if (Utility::InterSleep(lock, m_cv, toEvent.total_milliseconds() / 1000.0))
                        return;
                }

                size_t overlappingEvents = CountOverlappingEvents(event);
                if (overlappingEvents)
                {
                    m_logger.info(
                        "Capturing event [#{} \"{}\"] and {} overlapping",
                        event->id(), event->name(), overlappingEvents
                    );
                }
                else
                {
                    m_logger.info(
                        "Capturing event [#{} \"{}\"]",
                        event->id(), event->name()
                    );
                }

                capture(std::move(event));
                m_queue.pop_front();
            }
            LogGenerationResult(generateEvents(m_lastEvent->timestamp().date() + dt::days(1)));
        }
    }
    catch (const std::exception& error)
    {
        m_logger.critical("Capture thread exception: \"{}\"", error.what());
        m_logger.critical("Capture thread is terminating");

        std::lock_guard lock(m_mutex);
        m_threadStatus = ThreadStatus::Idle;
    }
}

size_t Capture::Master::capture(Event::Pointer&& event, bool expired)
{
    size_t eventsCaptured = 0;
    if (!expired)
    {
        for (Event* captureEvent = event.get(); captureEvent; captureEvent = captureEvent->overlapping().get())
        {
            /* Event capture */
            ++eventsCaptured;
        }
    }
    else
    {
        for (Event* captureEvent = event.get(); captureEvent; captureEvent = captureEvent->overlapping().get())
        {
            std::string eventPath = fmt::format(
                "{}/{}/{}.event",
                CaptureDirectory,
                captureEvent->name(),
                Utility::ToFilename(captureEvent->timestamp())
            );
            captureEvent->save(eventPath);
            ++eventsCaptured;
        }
    }

    m_lastEvent = std::move(event);
    m_lastEvent->save(fmt::format("{}/{}", CaptureDirectory, LastEventFile));
    return eventsCaptured;
}

Capture::Master::GenerationResult Capture::Master::generateEvents(dt::date date)
{
    // Clear queue and generate events
    GenerationResult result = { date };
    m_queue.clear();
    Event::Generate(date, m_queue);

    // Sort events and assign IDs
    std::sort(
        m_queue.begin(),
        m_queue.end(),
        [](const Event::Pointer& left, const Event::Pointer& right) { return left->timestamp() < right->timestamp(); }
    );
    for (size_t index = 0, size = m_queue.size(); index < size; ++index)
        m_queue[index]->id(static_cast<int>(index + 1));

    // Erase already captured events
    std::erase_if(m_queue, [this](const Event::Pointer& event) { return event->timestamp() <= m_lastEvent->timestamp(); });
    result.generated = m_queue.size();
    if (result.generated == 0)
        return result;

    // Manage overlapped events
    for (size_t masterIndex = 0; masterIndex < m_queue.size() - 1; ++masterIndex)
    {
        size_t nextMasterIndex = masterIndex + 1;
        for (; nextMasterIndex < m_queue.size(); ++nextMasterIndex)
        {
            pt::time_duration eventsDelta = m_queue[nextMasterIndex]->timestamp() - m_queue[masterIndex]->timestamp();
            if (eventsDelta.total_milliseconds() > Config::Instance->timeReserve())
                break;
        }

        for (size_t slaveIndex = nextMasterIndex - 1; slaveIndex > masterIndex; --slaveIndex)
        {
            m_queue[slaveIndex - 1]->overlapping(std::move(m_queue[slaveIndex]));
            m_queue.erase(m_queue.begin() + slaveIndex);
            ++result.mapped;
        }
    }

    // Manage expired events
    while (!m_queue.empty())
    {
        Event::Pointer& event = m_queue[0];
        pt::time_duration toEvent = event->timestamp() - pt::microsec_clock::local_time();
        if (toEvent.total_milliseconds() > Config::Instance->timeReserve())
            break;

        result.expired += capture(std::move(event), true);
        m_queue.pop_front();
    }

    return result;
}

void Capture::Master::printQueue()
{
    fmt::print("{:<3} {:<10} {:<16} {:>4}\n", "ID", "Name", "Timestamp", "Mapped");
    if (m_queue.empty())
    {
        fmt::print("No events\n");
        return;
    }

    size_t eventCount = 0;
    for (const Event::Pointer& event : m_queue)
    {
        size_t overlappingCount = CountOverlappingEvents(event);
        eventCount += 1 + overlappingCount;
        fmt::print(
            "{:>3} {:<10} {:<16} {:>4}\n",
            event->id(), event->name(),
            Utility::ToString(event->timestamp()),
            overlappingCount ? std::to_string(overlappingCount) : std::string()
        );
    }
    fmt::print("{} event{}\n", eventCount, eventCount == 0 ? "" : "s");
}

Capture::Master::Master()
    : m_logger(Utility::CreateLogger("master"))
    , m_threadStatus(ThreadStatus::Idle)
{}

Capture::Master::~Master()
{
    stop();
}

bool Capture::Master::started()
{
    std::lock_guard lock(m_mutex);
    return m_threadStatus == ThreadStatus::Running;
}

void Capture::Master::start(bool blocking)
{
    {
        std::lock_guard lock(m_mutex);
        if (m_threadStatus == ThreadStatus::Running)
            return;
    }
    m_logger.info("Starting capture");

    m_threadStatus = ThreadStatus::Running;
    m_thread = std::thread(&Master::captureFunction, this);
    if (blocking)
        m_thread.join();
}

void Capture::Master::stop()
{
    std::unique_lock lock(m_mutex);
    if (m_threadStatus != ThreadStatus::Running)
    {
        if (m_thread.joinable())
            m_thread.join();
        return;
    }
    m_threadStatus = ThreadStatus::Stopped;

    lock.unlock();
    m_cv.notify_one();
    if (m_thread.joinable())
        m_thread.join();
    lock.lock();

    m_threadStatus = ThreadStatus::Idle;
    m_logger.info("Stopped capture");
}

} // namespace kc

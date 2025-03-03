#include "sensors/recorder.hpp"
using namespace cp::Sensors::RecorderConst;

#include "common/utility.hpp"

namespace cp {

/*
*   std::make_unique() needs public constructor, but the Recorder class uses singleton pattern.
*   This is why operator new is used instead.
*/
const std::unique_ptr<Sensors::Recorder> Sensors::Recorder::Instance(new Recorder);

Sensors::Recorder::Record Sensors::Recorder::Record::operator-(const Record& other) const {
    Record result = *this;
    result.timestamp = other.timestamp;

    if (result.external && other.external) {
        *result.external -= *other.external;
        result.external->round();
    }
    else {
        result.external.reset();
    }

    if (result.internal && other.internal) {
        *result.internal -= *other.internal;
        result.internal->round();
    }
    else {
        result.internal.reset();
    }

    return result;
}

Sensors::Recorder::Recorder()
    : m_logger(Utility::CreateLogger("recorder"))
    , m_thread(&Recorder::recordFunction, this)
{}

Sensors::Recorder::~Recorder() {
    {
        std::lock_guard lock(m_mutex);
        m_threadStatus = ThreadStatus::Stopped;
    }

    m_cv.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void Sensors::Recorder::measure(Record& record, Location location) {
    try {
        Measurement measurement = Measure(location, MeasurementIterations);

        std::lock_guard lock(m_mutex);
        if (location == Location::Internal) {
            record.internal.emplace(measurement);
        }
        else {
            record.external.emplace(measurement);
        }
    }
    catch (const std::runtime_error& error) {
        m_logger.error(
            "Couldn't measure {} sensors: {}",
            location == Location::Internal ? "internal" : "external",
            error.what()
        );
    }
}

void Sensors::Recorder::recordFunction() {
    while (true) {
        std::unique_lock lock(m_mutex);
        if (m_threadStatus == ThreadStatus::Stopped) {
            return;
        }

        lock.unlock();
        Record record = { Utility::TimestampWithoutSeconds(pt::second_clock::local_time()) + pt::minutes(1) };
        std::thread externalMeasurementThread(&Recorder::measure, this, std::ref(record), Location::External);
        std::thread internalMeasurementThread(&Recorder::measure, this, std::ref(record), Location::Internal);
        externalMeasurementThread.join();
        internalMeasurementThread.join();
        lock.lock();

        m_history.push_back(record);
        if (m_history.size() > MaxHistorySize) {
            m_history.pop_front();
        }
        m_cv.notify_all();

        if (m_threadStatus == ThreadStatus::Stopped) {
            return;
        }
        pt::time_duration toNextHalfMinute = Utility::TimeToNextHalfMinute();
        if (toNextHalfMinute.total_seconds() <= 30) {
            toNextHalfMinute += pt::minutes(1);
        }
        if (Utility::InterSleep(lock, m_cv, toNextHalfMinute.total_milliseconds() / 1000.0)) {
            return;
        }
    }
}

void Sensors::Recorder::awaitHistory(std::unique_lock<std::mutex>& lock) {
    if (m_history.empty()) {
        m_cv.wait(lock);
    }
}

Sensors::Recorder::Record Sensors::Recorder::last() {
    std::unique_lock lock(m_mutex);
    awaitHistory(lock);
    return *(m_history.end() - 1);
}

Sensors::Recorder::Record Sensors::Recorder::trend(int interval) {
    std::unique_lock lock(m_mutex);
    awaitHistory(lock);
    if (interval >= m_history.size()) {
        interval = static_cast<int>(m_history.size()) - 1;
    }

    const auto& current = m_history.end() - 1;
    const auto& previous = current - interval;
    return *current - *previous;
}

Sensors::Recorder::HistoryHandle Sensors::Recorder::historyHandle() {
    std::unique_lock lock(m_mutex);
    awaitHistory(lock);
    return { std::move(lock), m_history };
}

} // namespace cp

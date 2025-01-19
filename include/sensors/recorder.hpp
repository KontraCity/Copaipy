#pragma once

#include <memory>
#include <deque>
#include <optional>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <boost/date_time.hpp>
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

#include <spdlog/spdlog.h>

#include "sensors/sensors.hpp"

namespace cp {

namespace Sensors {
    namespace RecorderConst {
        /*
        *   Maximum amount of records in recorder's history.
        *   Here it's the amount of records for one week.
        */
        constexpr int MaxHistorySize = 7 * 24 * 60;

        /*
        *   Counts of sensors measurement iterations
        *   More: more accurate readings, slower measurement
        *   Less: less accurate readings, faster measurement
        */
        constexpr int MeasurementIterations = 5;
    }

    class Recorder {
    public:
        static const std::unique_ptr<Recorder> Instance;

        struct Record {
            pt::ptime timestamp;
            std::optional<Measurement> external;    // Measurement error occured if the optional is empty
            std::optional<Measurement> internal;    // Measurement error occured if the optional is empty

            Record operator-(const Record& other) const;
        };

        using History = std::deque<Record>;

        struct HistoryHandle {
            std::unique_lock<std::mutex> lock;
            const History& history;
        };

    private:
        enum class ThreadStatus {
            Idle,
            Running,
            Stopped,
        };

    private:
        spdlog::logger m_logger;
        std::mutex m_mutex;
        std::thread m_thread;
        ThreadStatus m_threadStatus = ThreadStatus::Idle;
        std::condition_variable m_cv;
        History m_history;

    private:
        Recorder();

    public:
        ~Recorder();

    private:
        void measure(Record& record, Location location);

        void recordFunction();

        void awaitHistory(std::unique_lock<std::mutex>& lock);

    public:
        Record last();

        Record trend(int interval = 60);

        HistoryHandle historyHandle();
    };
}

} // namespace cp

#pragma once

// STL modules
#include <memory>
#include <deque>
#include <optional>
#include <mutex>
#include <thread>
#include <condition_variable>

// Library Boost.Date_Time
#include <boost/date_time.hpp>

// Custom modules
#include "common/utility.hpp"
#include "sensors/sensors.hpp"

namespace kc {

/* Namespace aliases and imports */
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

namespace Sensors
{
    namespace RecorderConst
    {
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

    class Recorder
    {
    public:
        // Singleton instance
        static const std::unique_ptr<Recorder> Instance;

        struct Record
        {
            pt::ptime timestamp;
            std::optional<Measurement> external;    // Measurement error occured if the optional is empty
            std::optional<Measurement> internal;    // Measurement error occured if the optional is empty

            /// @brief Get substraction result of other record from this record
            /// @param other The other record to substract
            /// @return Result of substraction
            Record operator-(const Record& other) const;
        };

        // Measurement history
        using History = std::deque<Record>;

    private:
        enum class ThreadStatus
        {
            Idle,
            Running,
            Stopped,
        };

    private:
        spdlog::logger m_logger;
        std::mutex m_mutex;
        std::thread m_thread;
        ThreadStatus m_threadStatus;
        std::condition_variable m_cv;
        History m_history;

    private:
        /// @brief Measure sensors to record
        /// @param record The record to measure sensors to
        /// @param location The location to measure sensors in
        void measure(Record& record, Location location);

        /// @brief Record thread implementation
        void recordFunction();

        /// @brief Wait for history to have at least one record
        /// @param lock Acquired mutex lock
        void awaitHistory(std::unique_lock<std::mutex>& lock);

        /// @brief Initialize recorder and start record thread
        Recorder();

    public:
        ~Recorder();

        /// @brief Get last measurement record
        /// @return Last measurement record
        Record last();

        /// @brief Get change trend for an interval
        /// @param interval The interval in minutes
        /// @return Change trend for an interval
        Record trend(int interval = 60);

        /// @brief Get whole recorder history
        /// @return Whole recorder history
        History history();

        /// @brief Get recorder history
        /// @param count Amount of records to get
        /// @return Recorder history
        History history(int count);
    };
}

} // namespace kc

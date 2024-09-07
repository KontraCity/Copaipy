#pragma once

// STL modules
#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <cmath>
#include <mutex>
#include <condition_variable>

// Library Boost.Date_Time
#include <boost/date_time.hpp>

// Library {fmt}
#include <fmt/format.h>

// Custom modules
#include "common/config.hpp"
#include "common/sensors.hpp"
#include "display/master.hpp"

namespace kc {

/* Namespace aliases and imports */
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

namespace Display
{
    class Ui : private Master
    {
    public:
        // Shared display UI instance pointer
        using Pointer = std::shared_ptr<Ui>;

        struct Message
        {
            struct Submessage
            {
                std::string string;
                double delay = 3.0;
                uint32_t blinks = 0;
            };

            std::string header;
            std::vector<Submessage> submessages;
        };

    private:
        enum class ThreadStatus
        {
            Idle,
            Running,
            Stopped,
        };

    private:
        /// @brief Get time to next minute
        /// @return Time to next minute
        static pt::time_duration TimeToNextMinute();

    private:
        std::deque<Message> m_queue;
        pt::ptime m_eventTimestamp;

        std::mutex m_mutex;
        std::mutex m_updateMutex;
        std::condition_variable m_cv;

        std::thread m_updateThread;
        ThreadStatus m_updateThreadStatus;
        std::thread m_messageThread;
        ThreadStatus m_messageThreadStatus;

    private:
        /// @brief Update duration to event timestamp on the display
        /// @param now Current time
        void updateEventTimestampDuration(pt::ptime now);

        /// @brief Update thread implementation
        void updateFunction();

        /// @brief Message thread implementation
        void messageFunction();

    public:
        /// @brief Initialize display UI
        Ui();

        ~Ui();

        /// @brief Check if display is enabled
        /// @return True if display is enabled
        bool enabled();

        /// @brief Enable display
        void enable();

        /// @brief Disable display
        void disable();

        /// @brief Show message on the display
        /// @param message The message to show
        void showMessage(const Message& message);

        /// @brief Update event timestamp
        /// @param timestamp The timestamp to update to
        void updateEventTimestamp(pt::ptime timestamp);
    };
}

} // namespace kc

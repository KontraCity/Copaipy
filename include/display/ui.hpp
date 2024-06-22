#pragma once

// STL modules
#include <memory>
#include <string>
#include <vector>
#include <deque>
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
            std::string header;
            std::vector<std::string> submessages;
            uint32_t delayInSeconds = 3;
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
        Config::Pointer m_config;
        std::deque<Message> m_queue;

        std::mutex m_mutex;
        std::mutex m_updateMutex;
        std::condition_variable m_cv;

        std::thread m_updateThread;
        ThreadStatus m_updateThreadStatus;
        std::thread m_messageThread;
        ThreadStatus m_messageThreadStatus;

    private:
        /// @brief Update thread implementation
        void updateFunction();

        /// @brief Message thread implementation
        void messageFunction();

    public:
        /// @brief Initialize display UI
        /// @param config Initialized config
        Ui(Config::Pointer config);

        ~Ui();

        /// @brief Turn on the display and start display UI
        void start();

        /// @brief Stop display UI and turn off the display
        void stop();

        /// @brief Show message on the display
        /// @param message The message to show
        void showMessage(const Message& message);
    };
}

} // namespace kc

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
#include "capture/event.hpp"
#include "common/config.hpp"
#include "common/utility.hpp"
#include "display/master.hpp"
#include "sensors/recorder.hpp"

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

        struct Screen
        {
            std::string line1;
            std::string line2;
            double delay = 5.0;
            uint32_t blinks = 0;
        };

        using Message = std::vector<Screen>;

    private:
        enum class ThreadStatus
        {
            Idle,
            Running,
            Stopped,
        };

    private:
        /// @brief Get change trend symbol
        /// @param trend The change trend to get symbol for
        /// @return Change trend symbol
        char TrendSymbol(double trend);

    private:
        std::deque<Message> m_queue;
        Capture::Event::Pointer m_nextEvent;

        std::mutex m_mutex;
        std::mutex m_updateMutex;
        std::condition_variable m_cv;

        std::thread m_updateThread;
        ThreadStatus m_updateThreadStatus;
        std::thread m_messageThread;
        ThreadStatus m_messageThreadStatus;

    private:
        /// @brief Update next event info on the display
        /// @param now Current time
        void updateNextEventInfo(pt::ptime now);

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

        /// @brief Update next event
        /// @param event The event to update to
        void updateNextEvent(Capture::Event* event);
    };
}

} // namespace kc

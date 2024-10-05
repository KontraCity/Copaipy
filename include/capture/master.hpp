#pragma once

// STL modules
#include <memory>
#include <algorithm>
#include <filesystem>
#include <mutex>
#include <thread>
#include <condition_variable>

// Custom modules
#include "capture/event.hpp"
#include "common/camera.hpp"
#include "common/config.hpp"
#include "common/stopwatch.hpp"
#include "common/utility.hpp"
#include "display/ui.hpp"

namespace kc {

namespace Capture
{
    namespace MasterConst
    {
        constexpr const char* CaptureDirectory = "Capture";
        constexpr const char* LastEventFile = "last.event";
    }

    class Master
    {
    public:
        // Shared capture master instance pointer
        using Pointer = std::shared_ptr<Master>;

    private:
        enum class ThreadStatus
        {
            Idle,
            Running,
            Stopped,
        };

        struct GenerationResult
        {
            dt::date date;
            size_t generated = 0;
            size_t mapped = 0;
            size_t expired = 0;
        };

        struct CaptureResult
        {
            size_t eventsCaptured = 0;  // Count of events captured (including overlapped events)
            size_t timeElapsed = 0;     // Amount of time it took to make the capture in milliseconds
            size_t savedSize = 0;       // Total size of created capture file(s) in bytes
        };

    private:
        /// @brief Create capture filesystem
        /// @throw std::runtime_error if internal error occurs
        /// @return "Start" event
        Event::Pointer CreateCaptureFilesystem();

        /// @brief Count overlapping events
        /// @param event The event which overlapping events to count
        /// @return Count of overlapping events
        size_t CountOverlappingEvents(const Event::Pointer& event);

        /// @brief Log generation result
        /// @param result The result to log
        void LogGenerationResult(const GenerationResult& result);

    private:
        spdlog::logger m_logger;
        Display::Ui::Pointer m_displayUi;
        Camera m_camera;
        GenerationResult m_lastGenerationResult;
        Event::Queue m_queue;
        Event::Pointer m_lastEvent;

        std::mutex m_mutex;
        std::thread m_thread;
        ThreadStatus m_threadStatus;
        std::condition_variable m_cv;

    private:
        /// @brief Capture thread implementation
        void captureFunction();

        /// @brief Sleep to timestamp
        /// @param timestamp The timestamp to sleep to
        /// @param subtractTimeReserve Whether or not to subtract time reserve from sleep
        /// @return True if thread was stopped or sleep was interrupted
        bool sleepToTimestamp(pt::ptime timestamp, bool subtractTimeReserve = false);

        /// @brief Capture event(s)
        /// @param event The event to capture
        /// @param expired Whether to capture event as expired or not
        /// @throw std::runtime_error if internal error occurs
        /// @return Capture result
        CaptureResult capture(Event::Pointer&& event, bool expired = false);

        /// @brief Generate events for date
        /// @param date The date to generate events for
        void generateEvents(dt::date date);

        /// @brief Print event queue
        void printQueue();

    public:
        /// @brief Initialize capture master
        /// @param displayUi Initialized display UI
        /// @throw std::runtime_error if internal error occurs
        Master(Display::Ui::Pointer displayUi);

        ~Master();

        /// @brief Check if capture is started
        /// @return True if capture is started
        bool started();

        /// @brief Start capture
        /// @param blocking Whether to start in current thread or not
        void start(bool blocking = false);

        /// @brief Stop capture
        void stop();
    };
}

} // namespace kc

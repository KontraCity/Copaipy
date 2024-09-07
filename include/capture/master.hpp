#pragma once

// STL modules
#include <filesystem>
#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>

// Custom modules
#include "capture/event.hpp"
#include "common/config.hpp"
#include "common/utility.hpp"

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
        Event::Queue m_queue;
        Event::Pointer m_lastEvent;

        std::mutex m_mutex;
        std::thread m_thread;
        ThreadStatus m_threadStatus;
        std::condition_variable m_cv;

    private:
        /// @brief Capture thread implementation
        void captureFunction();

        /// @brief Capture event(s)
        /// @param event The event to capture
        /// @param expired Whether to capture event as expired or not
        /// @throw std::runtime_error if internal error occurs
        /// @return Count of captured events
        size_t capture(Event::Pointer&& event, bool expired = false);

        /// @brief Generate events for date
        /// @param date The date to generate events for
        GenerationResult generateEvents(dt::date date);

        /// @brief Print event queue
        void printQueue();

    public:
        /// @brief Initialize capture master
        /// @throw std::runtime_error if internal error occurs
        Master();

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

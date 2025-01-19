#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <spdlog/spdlog.h>

#include "capture/event.hpp"
#include "common/camera.hpp"
#include "display/ui.hpp"

namespace cp {

namespace Capture {
    namespace MasterConst {
        constexpr const char* CaptureDirectory = "Capture";
        constexpr const char* LastEventFile = "last.event";
    }

    class Master {
    public:
        using Pointer = std::shared_ptr<Master>;

        enum class ThreadStatus {
            Idle,
            Running,
            Stopped,
        };

        struct GenerationResult {
            dt::date date;
            size_t generated = 0;
            size_t mapped = 0;
            size_t expired = 0;
        };

        struct CaptureResult {
            size_t eventsCaptured = 0;  // Count of events captured (including overlapped events)
            size_t timeElapsed = 0;     // Amount of time it took to make the capture in milliseconds
            size_t savedSize = 0;       // Total size of created capture file(s) in bytes
        };

    private:
        spdlog::logger m_logger;
        Display::Ui::Pointer m_displayUi;
        Camera m_camera;
        GenerationResult m_lastGenerationResult;
        Event::Queue m_queue;
        Event::Pointer m_lastEvent;

        mutable std::mutex m_mutex;
        std::thread m_thread;
        ThreadStatus m_threadStatus = ThreadStatus::Idle;
        std::condition_variable m_cv;

    public:
        Master(Display::Ui::Pointer displayUi);

        ~Master();

    private:
        void captureFunction();

        bool sleepToTimestamp(pt::ptime timestamp, bool subtractTimeReserve = false);

        CaptureResult capture(Event::Pointer&& event, bool expired = false);

        void generateEvents(dt::date date);

        void printQueue();

    public:
        inline operator bool() const {
            std::lock_guard lock(m_mutex);
            return m_threadStatus == ThreadStatus::Running;
        };

        void enable(bool blocking = false);

        void disable();
    };
}

} // namespace cp

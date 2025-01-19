#pragma once

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>

#include <boost/date_time.hpp>
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

#include "capture/event.hpp"
#include "display/master.hpp"

namespace cp {

namespace Display {
    class Ui : private Master {
    public:
        using Pointer = std::shared_ptr<Ui>;

        struct Screen {
            std::string line1;
            std::string line2;
            double delay = 5.0;
            uint32_t blinks = 0;
        };

        using Message = std::vector<Screen>;

    private:
        enum class ThreadStatus {
            Idle,
            Running,
            Stopped,
        };

    private:
        std::deque<Message> m_queue;
        Capture::Event::Pointer m_nextEvent;

        mutable std::mutex m_mutex;
        std::mutex m_updateMutex;
        std::condition_variable m_cv;

        std::thread m_updateThread;
        ThreadStatus m_updateThreadStatus = ThreadStatus::Idle;
        std::thread m_messageThread;
        ThreadStatus m_messageThreadStatus = ThreadStatus::Idle;

    public:
        Ui();

        ~Ui();

    private:
        void updateNextEventInfo(pt::ptime now);

        void updateFunction();

        void messageFunction();

    public:
        void enable();

        void disable();

        void showMessage(const Message& message);

        void updateNextEvent(Capture::Event* event);

    public:
        inline operator bool() const {
            std::lock_guard lock(m_mutex);
            return (m_updateThreadStatus == ThreadStatus::Running);
        }
    };
}

} // namespace cp

#pragma once

#include <memory>
#include <deque>
#include <set>
#include <string>

#include <boost/date_time.hpp>
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

namespace cp {

namespace Capture {
    namespace EventConst {
        namespace Objects {
            constexpr const char* Name = "name";
            constexpr const char* ShortName = "short_name";
            constexpr const char* Timestamp = "timestamp";
        }
    }

    class Event {
    public:
        using Pointer = std::unique_ptr<Event>;

        using Queue = std::deque<Pointer>;

        using Tasks = std::set<std::string>;

    public:
        static void Generate(dt::date date, Queue& queue);

        static const Tasks& GetTasks();

    private:
        int m_id = -1;
        std::string m_name;
        std::string m_shortName;
        pt::ptime m_timestamp;
        Pointer m_overlapping;

    public:
        Event(const std::string& name, const std::string& shortName, pt::ptime timestamp);

        Event(const std::string& filename);

    public:
        void save(const std::string& filename) const;

        std::string summary(int length = -1) const;

    public:
        inline int id() const {
            return m_id;
        }

        inline int& id() {
            return m_id;
        }

        inline const std::string& name() const {
            return m_name;
        }

        inline const std::string& shortName() const {
            return m_shortName;
        }

        inline pt::ptime timestamp() const {
            return m_timestamp;
        }

        inline const Pointer& overlapping() const {
            return m_overlapping;
        }

        inline Pointer& overlapping() {
            return m_overlapping;
        }
    };
}

} // namespace cp

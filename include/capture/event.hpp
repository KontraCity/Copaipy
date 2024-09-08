#pragma once

// STL modules
#include <string>
#include <memory>
#include <deque>
#include <set>
#include <fstream>
#include <stdexcept>

// Library nlohmann::json
#include <nlohmann/json.hpp>

// Library Boost.Date_Time
#include <boost/date_time.hpp>

// Library {fmt}
#include <fmt/format.h>

// Custom modules
#include "common/astronomy.hpp"
#include "common/config.hpp"
#include "common/utility.hpp"

namespace kc {

/* Namespace aliases and imports */
using nlohmann::json;
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

namespace Capture
{
    namespace EventConst
    {
        namespace Objects
        {
            constexpr const char* Name = "name";
            constexpr const char* ShortName = "short_name";
            constexpr const char* Timestamp = "timestamp";
        }
    }

    class Event
    {
    public:
        // Unique event instance pointer
        using Pointer = std::unique_ptr<Event>;

        // Capture event queue
        using Queue = std::deque<Pointer>;

        // Task names list
        using Tasks = std::set<std::string>;

    public:
        /// @brief Generate events to queue for a given date
        /// @param date The date to generate events for
        /// @param queue The queue to generate events to
        static void Generate(dt::date date, Queue& queue);

        /// @brief Get all tasks
        /// @return All tasks
        static const Tasks& GetTasks();

    private:
        int m_id;
        std::string m_name;
        std::string m_shortName;
        pt::ptime m_timestamp;
        Pointer m_overlapping;

    public:
        /// @brief Create an event
        /// @param name Event name
        /// @param shortName Short event name
        /// @param timestamp Event timestamp
        Event(const std::string& name, const std::string& shortName, pt::ptime timestamp);

        /// @brief Parse event from file
        /// @param filename Name of the file to parse
        /// @throw std::runtime_error if internal error occurs
        Event(const std::string& filename);

        /// @brief Save event to file
        /// @param filename Name of the file to save
        /// @throw std::runtime_error if internal error occurs
        void save(const std::string& filename) const;

        /// @brief Get event summary
        /// @param length Summary string length
        /// @return Event summary
        std::string summary(int length = -1) const;

        /// @brief Get event ID
        /// @return Event ID
        inline int id() const
        {
            return m_id;
        }

        /// @brief Set event ID
        /// @param id The ID to set
        inline void id(int id)
        {
            m_id = id;
        }

        /// @brief Get event name
        /// @return Event name
        inline const std::string& name() const
        {
            return m_name;
        }

        /// @brief Get short event name
        /// @return Short event name
        inline const std::string& shortName() const
        {
            return m_shortName;
        }

        /// @brief Get event timestamp
        /// @return Event timestamp
        inline pt::ptime timestamp() const
        {
            return m_timestamp;
        }

        /// @brief Get overlapping event
        /// @return Overlapping event
        inline const Pointer& overlapping() const
        {
            return m_overlapping;
        }

        /// @brief Set overlapping event
        /// @param event The overlapping event to set
        inline void overlapping(Pointer&& event)
        {
            m_overlapping = std::move(event);
        }
    };
}

} // namespace kc

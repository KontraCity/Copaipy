#include "capture/event.hpp"
using namespace kc::Capture::EventConst;

namespace kc {

void Capture::Event::Generate(dt::date date, Queue& queue)
{
    /*
    *   "Main" task:
    *       -> Target: 60 FPS timelapse where 1 second = 1 real day.
    *       -> Capture: Evenly, 60 captures every day (one every 24 minutes).
    */
    pt::time_duration step = pt::time_duration(24, 0, 0) / 60;
    for (pt::ptime timestamp = pt::ptime(date) + step / 2; timestamp.date() == date; timestamp += step)
        queue.emplace_back(std::make_unique<Event>("Main", "MA", timestamp));

    /*
    *   "Midnight" task:
    *       -> Target: 60 FPS timelapse where 1 second = 60 real days.
    *       -> Capture: Every midnight, 1 capture every day (one every 24 hours).
    */
    queue.emplace_back(std::make_unique<Event>("Midnight", "MN", pt::ptime(date)));

    /*
    *   "Midday" task:
    *       -> Target: 60 FPS timelapse where 1 second = 60 real days.
    *       -> Capture: Every midday, 1 capture every day (one every 24 hours).
    */
    queue.emplace_back(std::make_unique<Event>("Midday", "MD", pt::ptime(date, pt::time_duration(12, 0, 0))));

    /*
    *   "Sunrise" task:
    *       -> Target: 60 FPS timelapse where 1 second = 60 real days.
    *       -> Capture: Every sunrise, 1 capture every day.
    */
    pt::ptime sunrise = Astronomy::CalculateSunrise(date);
    queue.emplace_back(std::make_unique<Event>("Sunrise", "SR", sunrise));

    /*
    *   "Sunset" task:
    *       -> Target: 60 FPS timelapse where 1 second = 60 real days.
    *       -> Capture: Every sunset, 1 capture every day.
    */
    pt::ptime sunset = Astronomy::CalculateSunset(date);
    queue.emplace_back(std::make_unique<Event>("Sunset", "SS", sunset));

    /*
    *   "Day" task:
    *       -> Target: 60 FPS timelapse where 1 second = 10 real days.
    *       -> Capture: Evenly between sunrises and sunsets, 6 captures every day.
    */
    step = (sunset - sunrise) / 6;
    for (pt::ptime timestamp = sunrise + step / 2; timestamp < sunset; timestamp += step)
        queue.emplace_back(std::make_unique<Event>("Day", "DA", timestamp));

    /*
    *   "Night" task:
    *       -> Target: 60 FPS timelapse where 1 second = 10 real days.
    *       -> Capture: Evenly between sunsets and sunrises, 6 captures every day.
    */
    step = (pt::time_duration(24, 0, 0) - (sunset - sunrise)) / 6;
    for (pt::ptime timestamp = sunrise - step / 2; timestamp.date().day() == sunrise.date().day(); timestamp -= step)
        queue.emplace_back(std::make_unique<Event>("Night", "NI", timestamp));
    for (pt::ptime timestamp = sunset + step / 2; timestamp.date().day() == sunset.date().day(); timestamp += step)
        queue.emplace_back(std::make_unique<Event>("Night", "NI", timestamp));
}

const Capture::Event::Tasks& Capture::Event::GetTasks()
{
    static Tasks tasks;
    if (tasks.empty())
    {
        Queue queue;
        Generate(dt::day_clock::local_day(), queue);
        for (const Event::Pointer& event : queue)
            tasks.insert(event->name());
    }
    return tasks;
}

Capture::Event::Event(const std::string& name, const std::string& shortName, pt::ptime timestamp)
    : m_id(-1)
    , m_name(name)
    , m_shortName(shortName)
    , m_timestamp(timestamp)
{}

Capture::Event::Event(const std::string& filename)
    : m_id(-1)
{
    std::ifstream file(filename);
    if (!file)
    {
        throw std::runtime_error(fmt::format(
            "kc::Capture::Event::save(): "
            "Couldn't open file \"{}\"",
            filename
        ));
    }

    try
    {
        const json eventJson = json::parse(file);
        m_name = eventJson.at(Objects::Name);
        m_shortName = eventJson.at(Objects::ShortName);
        m_timestamp = pt::from_iso_extended_string(eventJson.at(Objects::Timestamp));
    }
    catch (const json::exception&)
    {
        throw std::runtime_error(fmt::format(
            "kc::Capture::Event::Event(): "
            "Couldn't parse event file JSON"
        ));
    }
    catch (const boost::exception&)
    {
        throw std::runtime_error(fmt::format(
            "kc::Capture::Event::Event(): "
            "Couldn't parse event file timestamp"
        ));
    }
}

void Capture::Event::save(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file)
    {
        throw std::runtime_error(fmt::format(
            "kc::Capture::Event::save(): "
            "Couldn't create file \"{}\"",
            filename
        ));
    }

    json eventJson;
    eventJson[Objects::Name] = m_name;
    eventJson[Objects::ShortName] = m_shortName;
    eventJson[Objects::Timestamp] = pt::to_iso_extended_string(m_timestamp);
    file << eventJson.dump(4) << '\n';
}

std::string Capture::Event::summary(int length) const
{
    std::string result = fmt::format("[#{} {}]", m_id, m_name);
    if (length <= 0)
        return result;

    int minLength = static_cast<int>(fmt::format("[#{} ]", m_id).length());
    if (length <= minLength)
        return Utility::Truncate(result, length);

    return fmt::format(
        "[#{} {:>{}}]",
        m_id,
        Utility::Truncate(m_name, length - minLength),
        length - minLength
    );
}

} // namespace kc

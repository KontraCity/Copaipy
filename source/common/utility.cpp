#include "common/utility.hpp"

namespace kc {

spdlog::logger Utility::CreateLogger(const std::string& name, std::optional<bool> forceColor)
{
    static auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    if (forceColor)
    {
        /*
        *   This function is initialized when [forceColor] parameter is passed.
        *   This is why the sink is configured here.
        */
        sink->set_pattern("[%^%d.%m.%C %H:%M:%S %L%$] [%n] %v");
        if (*forceColor)
            sink->set_color_mode(spdlog::color_mode::always);
    }
    return { name, {sink} };
}

void Utility::Sleep(double seconds)
{
    std::this_thread::sleep_for(std::chrono::microseconds(static_cast<size_t>(seconds * 1'000'000)));
}

bool Utility::InterSleep(std::unique_lock<std::mutex>& lock, std::condition_variable& cv, double seconds)
{
    std::cv_status result = cv.wait_for(lock, std::chrono::microseconds(static_cast<size_t>(seconds * 1'000'000)));
    return result == std::cv_status::no_timeout;
}

double Utility::Round(double value, int decimalPlacesCount)
{
    double multiplier = std::pow(10, decimalPlacesCount);
    double result = std::round(value * multiplier) / multiplier;
    return (result == -0 ? 0 : result); // -0 doesn't make sense
}

double Utility::Limit(double value, double min, double max)
{
    if (value < min)
        value = min;
    if (value > max)
        value = max;
    return value;
}

int Utility::GetTimezoneOffset()
{
    pt::ptime localTime = pt::second_clock::local_time();
    pt::ptime universalTime = pt::second_clock::universal_time();
    return static_cast<int>(std::round((localTime - universalTime).total_seconds() / 3600.0));
}

std::string Utility::ToString(dt::date date)
{
    return fmt::format(
        "{:#02d}.{:#02d}.{:#02d}",
        static_cast<int>(date.day()), date.month().as_number(), static_cast<int>(date.year()) % 100
    );
}

std::string Utility::ToString(pt::time_duration duration, bool force)
{
    if (force)
        return fmt::format("{:#02d}:{:#02d}:{:#02d}", duration.hours(), duration.minutes(), duration.seconds());
    if (duration.hours())
        return fmt::format("{}:{:#02d}:{:#02d}", duration.hours(), duration.minutes(), duration.seconds());
    return fmt::format("{}:{:#02d}", duration.minutes(), duration.seconds());
}

std::string Utility::ToString(pt::ptime timestamp)
{
    return fmt::format("{} {}", ToString(timestamp.date()), ToString(timestamp.time_of_day(), true));
}

std::string Utility::ToFilename(pt::ptime timestamp)
{
    std::string result = fmt::format(
        "{:#04d}.{:#02d}.{:#02d} {:#02d}-{:#02d}-{:#02d}",
        static_cast<int>(timestamp.date().year()),
        timestamp.date().month().as_number(),
        static_cast<int>(timestamp.date().day()),
        timestamp.time_of_day().hours(),
        timestamp.time_of_day().minutes(),
        timestamp.time_of_day().seconds()
    );

    int milliseconds = timestamp.time_of_day().fractional_seconds() / 1000;
    if (milliseconds)
        result += fmt::format(".{:#03d}", milliseconds);
    return result;
}

std::string Utility::Truncate(const std::string& string, size_t maxLength, bool end)
{
    if (string.length() <= maxLength)
        return string;
    if (maxLength <= 2)
        return std::string(maxLength, '.');

    if (end)
        return (std::string(string.begin(), string.begin() + maxLength - 2) + "..");
    return (".." + std::string(string.end() - maxLength + 2, string.end()));
}

} // namespace kc

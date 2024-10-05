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

pt::time_duration Utility::TimeToNextMinute()
{
    pt::ptime now = pt::microsec_clock::local_time();
    pt::ptime nextMinute = now + pt::minutes(1);
    return TimestampWithoutSeconds(nextMinute) - now;
}

pt::time_duration Utility::TimeToNextHalfMinute()
{
    pt::time_duration result = TimeToNextMinute() + pt::seconds(30);
    if (result.total_seconds() > 60)
        result -= pt::minutes(1);
    return result;
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

int Utility::TimezoneOffset()
{
    pt::ptime localTime = pt::second_clock::local_time();
    pt::ptime universalTime = pt::second_clock::universal_time();
    return static_cast<int>(std::round((localTime - universalTime).total_seconds() / 3600.0));
}

pt::ptime Utility::TimestampWithoutSeconds(pt::ptime timestamp)
{
    return { timestamp.date(), pt::time_duration(timestamp.time_of_day().hours(), timestamp.time_of_day().minutes(), 0) };
}

int Utility::ToUnixTimestamp(pt::ptime timestamp)
{
    constexpr pt::ptime epoch(dt::date(1970, 1, 1));
    return static_cast<int>((timestamp - epoch).total_seconds());
}

bool Utility::IsDaylight(pt::ptime timestamp)
{
    static dt::date date;
    static pt::ptime sunriseTimestamp, sunsetTimestamp;
    if (timestamp.date() != date)
    {
        date = timestamp.date();
        sunriseTimestamp = Astronomy::CalculateSunrise(date);
        sunsetTimestamp = Astronomy::CalculateSunset(date);
    }

    if (timestamp < sunriseTimestamp)
        return false;
    if (timestamp <= sunsetTimestamp)
        return true;
    return false;
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

    int milliseconds = static_cast<int>(timestamp.time_of_day().fractional_seconds() / 1000.0);
    if (milliseconds)
        result += fmt::format(".{:#03d}", milliseconds);
    return result;
}

std::string Utility::ToReadableSize(size_t size, char suffix)
{
    double number = static_cast<double>(size);
    for (const char* unit : { "", "Ki", "Mi", "Gi" })
    {
        if (number < 1024.0)
            return fmt::format("{:.1f} {}{}", number, unit, suffix);
        number /= 1024.0;
    }
    return fmt::format("{:.1f} Ti{}", number, suffix);
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

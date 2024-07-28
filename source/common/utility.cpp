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

} // namespace kc

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

double Utility::Round(double value, int decimalPlacesCount)
{
    double multiplier = std::pow(10, decimalPlacesCount);
    double result = std::round(value * multiplier) / multiplier;
    return (result == -0 ? 0 : result); // -0 doesn't make sense
}

} // namespace kc

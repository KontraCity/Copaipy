#pragma once

#include <optional>
#include <mutex>
#include <condition_variable>

#include <spdlog/spdlog.h>

#include <boost/date_time.hpp>
namespace dt = boost::gregorian;
namespace pt = boost::posix_time;

namespace cp {

namespace Utility {
    spdlog::logger CreateLogger(const std::string& name, std::optional<bool> forceColor = {});

    void Sleep(double seconds);

    bool InterSleep(std::unique_lock<std::mutex>& lock, std::condition_variable& cv, double seconds);

    pt::time_duration TimeToNextMinute();

    pt::time_duration TimeToNextHalfMinute();

    double Round(double value, int decimalPlacesCount);

    double Limit(double value, double min, double max);

    int TimezoneOffset();

    pt::ptime TimestampWithoutSeconds(pt::ptime timestamp);

    int ToUnixTimestamp(pt::ptime timestamp);

    bool IsDaylight(pt::ptime timestamp);

    std::string ToString(dt::date date);

    std::string ToString(pt::time_duration duration, bool force = false);

    std::string ToString(pt::ptime timestamp);

    std::string ToFilename(pt::ptime timestamp);

    std::string ToReadableSize(size_t size, char suffix = 'B');

    std::string Truncate(const std::string& string, size_t maxLength, bool end = true);
}

} // namespace cp

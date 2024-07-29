#pragma once

// STL modules
#include <chrono>

namespace kc {

class Stopwatch
{
public:
    using Seconds = std::chrono::seconds;
    using Milliseconds = std::chrono::milliseconds;
    using Microseconds = std::chrono::microseconds;
    using Nanoseconds = std::chrono::nanoseconds;

private:
    std::chrono::high_resolution_clock::time_point m_start;

public:
    /// @brief Start stopwatch
    Stopwatch();

    /// @brief Reset stopwatch
    void reset();

    /// @brief Get elapsed time from stopwatch start/reset
    /// @tparam Unit to get elapsed time in
    /// @return Elapsed time
    template <typename Unit>
    inline uint64_t elapsed() const
    {
        return std::chrono::duration_cast<Unit>(std::chrono::high_resolution_clock::now() - m_start).count();
    }
};

} // namespace kc

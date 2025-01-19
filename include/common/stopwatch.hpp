#pragma once

#include <chrono>

namespace cp {

class Stopwatch {
private:
    using clock = std::chrono::high_resolution_clock;
    clock::time_point m_start;

public:
    Stopwatch()
        : m_start(clock::now())
    {}

public:
    inline void reset() {
        m_start = clock::now();
    }

    inline float seconds() const {
        return milliseconds() / 1000.0f;
    }

    inline float milliseconds() const {
        return microseconds() / 1000.0f;
    }

    inline size_t microseconds() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - m_start).count();
    }
};

} // namespace cp

#include "common/stopwatch.hpp"

namespace kc {

Stopwatch::Stopwatch()
{
    reset();
}

void Stopwatch::reset()
{
    m_start = std::chrono::high_resolution_clock::now();
}

} // namespace kc

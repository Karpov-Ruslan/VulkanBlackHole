#pragma once

#include <chrono>

namespace KRV {

// Seconds clock
class Clock final {
public:
    double Reset();
    double GetTime() const;

private:
    using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
    constexpr static auto now = std::chrono::high_resolution_clock::now;

    time_point start = now();
};

}

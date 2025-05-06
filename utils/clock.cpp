#include "clock.hpp"

namespace KRV {

double Clock::Reset() {
    double const ret = GetTime();
    start = now();
    return ret;
}

double Clock::GetTime() const {
    using duration = std::chrono::duration<double>;
    return duration(now() - start).count();
}

}
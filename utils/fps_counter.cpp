#include "fps_counter.hpp"

namespace KRV {

double FPSCounter::Reset() {
    double ret = GetMeanFPS();
    clock.Reset();
    numOfFrames = 0ULL;
    return ret;
}

double FPSCounter::GetMeanFPS() const {
    return static_cast<double>(numOfFrames)/clock.GetTime();
}

double FPSCounter::GetTime() const {
    return clock.GetTime();
}

void FPSCounter::IncreaseNumOfFrames(uint64_t count) {
    numOfFrames += count;
}


}

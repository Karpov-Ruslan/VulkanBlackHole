#include "clock.hpp"

namespace KRV {

// Wrapper around `Clock` class.
class FPSCounter final {
public:
    // Set `numOfFrames` to zero and reset `clock`.
    // Return value: GetMeanFPS();
    double Reset();
    double GetMeanFPS() const;
    double GetTime() const;
    void IncreaseNumOfFrames(uint64_t count);

private:
    Clock clock{};
    uint64_t numOfFrames = 0ULL;
};

}

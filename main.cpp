#include <app.hpp>

int main() {
    KRV::Core::Core core;
    KRV::App app(&core);
    app.DrawFrames();

    return 0;
}

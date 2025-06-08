#include "app.hpp"

#include "utils/window.hpp"
#include "utils/fps_counter.hpp"
#include <iostream>

namespace KRV {

App::App() {}

void App::RenderLoop() {
    FPSCounter fpsCounter;
    Window& window = Window::GetInstance();
    while (!window.ShouldClose()) {
        window.PollEvents();
        vulkanController.DrawFrame();
        if (fpsCounter.GetTime() > 1.0F) {
            std::cout << fpsCounter.Reset() << std::endl;
        }
        fpsCounter.IncreaseNumOfFrames(1U);
    }
}

}

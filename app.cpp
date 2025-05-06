#include "app.hpp"

#include "utils/window.hpp"

namespace KRV {

App::App() : VulkanController() {}

void App::RenderLoop() {
    Window& window = Window::GetInstance();
    while (!window.ShouldClose()) {
        window.PollEvents();
        DrawFrame();
    }
}

}

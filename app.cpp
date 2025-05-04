#include "app.hpp"

#include "my_vulkan/window.hpp"

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

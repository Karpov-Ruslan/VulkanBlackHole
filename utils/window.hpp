#pragma once

#include <vulkan/vulkan_core.h>

#include <windows.h>
#include <vulkan/vulkan_win32.h>

#include <GLFW/glfw3.h>

#include <vector>

namespace KRV {

// Abstract Window class
class Window final {
public:
    struct Events final {
        struct Keyboard final {
            bool W = false;
            bool A = false;
            bool S = false;
            bool D = false;
            bool E = false;
            bool Q = false;
            bool ARROW_UP = false;
            bool ARROW_DOWN = false;
            bool ARROW_LEFT = false;
            bool ARROW_RIGHT = false;
        };

        struct Mouse final {
            // TODO: Add mouse events
        };

        Keyboard keyboard;
        Mouse mouse;
    };

    static Window& GetInstance();

    std::vector<char const *> GetVulkanSurfaceExtensions();

    // Should be platform-independent
    VkWin32SurfaceCreateInfoKHR GetVulkanSurfaceCreateInfo();

    VkExtent2D GetWindowSize();

    bool ShouldClose();
    void PollEvents();
    Events const & GetEvents() const;

private:
    Window();

    Window(Window const &) = delete;
    Window& operator=(Window const &) = delete;
    Window(Window &&) = delete;
    Window& operator=(Window &&) = delete;

    ~Window();

    void ProcessEvents();

    Events events;
    GLFWwindow *window = nullptr;
};

}

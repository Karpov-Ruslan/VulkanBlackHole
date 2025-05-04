#pragma once

#include <windows.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace KRV {

// Abstract Window class
class Window final {
public:
    static Window& GetInstance();

    std::vector<char const *> GetVulkanSurfaceExtensions();

    // Should be platform-independent
    VkWin32SurfaceCreateInfoKHR GetVulkanSurfaceCreateInfo();

    VkExtent2D GetWindowSize();

    bool ShouldClose();
    void PollEvents();

private:
    Window();

    Window(Window const &) = delete;
    Window& operator=(Window const &) = delete;
    Window(Window &&) = delete;
    Window& operator=(Window &&) = delete;

    ~Window();

    GLFWwindow *window = nullptr;
};

}

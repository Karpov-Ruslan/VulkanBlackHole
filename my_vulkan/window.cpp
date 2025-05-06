#include "window.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <constants.hpp>

namespace KRV {

Window& Window::GetInstance() {
    static Window window;
    return window;
}

std::vector<char const *> Window::GetVulkanSurfaceExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (glfwExtensions == nullptr) {
        throw std::runtime_error("GLFW return nullptr as extensions");
    }

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
}

Window::Window() {
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error(R"(GLFW wasn't init)");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_SIZE_WIDTH, WINDOW_SIZE_HEIGHT, "Vulkan", nullptr, nullptr);

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

VkWin32SurfaceCreateInfoKHR Window::GetVulkanSurfaceCreateInfo() {
    // It is WinAPI, so I am scared of using `nullptr` instead of `NULL`.
    HINSTANCE hinstance = GetModuleHandle(NULL);
    HWND hwnd = glfwGetWin32Window(window);

    VkWin32SurfaceCreateInfoKHR surfaceCI {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0U,
        .hinstance = hinstance,
        .hwnd = hwnd
    };

    return surfaceCI;
}

VkExtent2D Window::GetWindowSize() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    return extent;
}

bool Window::ShouldClose() {
    return glfwWindowShouldClose(window);
}

void Window::PollEvents() {
    glfwPollEvents();
}


Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

}
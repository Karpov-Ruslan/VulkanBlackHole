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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
    return glfwWindowShouldClose(window) || events.keyboard.ESC;
}

void Window::PollEvents() {
    glfwPollEvents();
    ProcessEvents();
}

Window::Events const & Window::GetEvents() const {
    return events;
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::ProcessEvents() {
    // Keyboard
    events.keyboard.W = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
    events.keyboard.A = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS);
    events.keyboard.S = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
    events.keyboard.D = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);
    events.keyboard.E = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
    events.keyboard.Q = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS);
    events.keyboard.ESC = (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    events.keyboard.ARROW_UP = (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
    events.keyboard.ARROW_DOWN = (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS);
    events.keyboard.ARROW_LEFT = (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
    events.keyboard.ARROW_RIGHT = (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);

    // Mouse
    double curPos_x, curPos_y;
    glfwGetCursorPos(window, &curPos_x, &curPos_y);
    curPos_x /= WINDOW_SIZE_WIDTH_F;
    curPos_y /= WINDOW_SIZE_HEIGHT_F;

    events.mouse.DELTA_POS_X = curPos_x - events.mouse.POS_X;
    events.mouse.DELTA_POS_Y = curPos_y - events.mouse.POS_Y;
    events.mouse.POS_X = static_cast<float>(curPos_x);
    events.mouse.POS_Y = static_cast<float>(curPos_y);
}

}
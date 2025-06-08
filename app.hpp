#pragma once

#include "my_vulkan/vulkan_controller.hpp"

namespace KRV {

class App final {
public:
    App();

    App(App const &) = delete;
    App& operator=(App const &) = delete;
    App(App &&) = delete;
    App& operator=(App &&) = delete;

    ~App() = default;

    void RenderLoop();

private:
    VulkanController vulkanController{};
};

}

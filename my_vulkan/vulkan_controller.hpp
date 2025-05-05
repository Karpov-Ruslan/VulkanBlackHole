#pragma once

#include <vulkan/vulkan_core.h>
#include "vulkan_functions.hpp"
#include "core/core.hpp"
#include <vector>
#include <array>

namespace KRV {

class VulkanController {
public:
    VulkanController();

    VulkanController(VulkanController const &) = delete;
    VulkanController& operator=(VulkanController const &) = delete;
    VulkanController(VulkanController &&) = delete;
    VulkanController& operator=(VulkanController &&) = delete;

    ~VulkanController();

protected:
    static constexpr uint32_t FRAMES_IN_FLIGHT = 2U;

    struct QueueInfo {
        VkQueue queue = VK_NULL_HANDLE;
        std::array<VkSemaphore, FRAMES_IN_FLIGHT> canRender;
        std::array<VkSemaphore, FRAMES_IN_FLIGHT> canPresent;
    };

    struct SwapchainInfo {
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> images = {};
        VkFormat format = {};
        VkExtent2D extent = {};
    };

    struct CommandBufferInfo {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> commandBuffers;
        std::array<VkFence, FRAMES_IN_FLIGHT> commandBufferFences;
        uint32_t fif = 0U;
    };

    void InitInstance();
    void InitSurface();
    void InitPhysicalDevice();
    void InitQueueFamilyIndex();
    void InitDevice();
    void InitQueue();
    void InitSwapchain();
    void InitCommandBuffers();

    void RecordCommandBuffer(VkImage swapchainImage, uint32_t fif);
    void DrawFrame();

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0U;
    QueueInfo queueInfo = {};
    SwapchainInfo swapchainInfo = {};
    CommandBufferInfo commandBufferInfo = {};

    Core core{};
};

}

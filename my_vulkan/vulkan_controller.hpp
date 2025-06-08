#pragma once

#include <vulkan/vulkan_core.h>
#include "core/core.hpp"
#include <vector>
#include <array>

namespace KRV {

class VulkanController final {
public:
    VulkanController();

    VulkanController(VulkanController const &) = delete;
    VulkanController& operator=(VulkanController const &) = delete;
    VulkanController(VulkanController &&) = delete;
    VulkanController& operator=(VulkanController &&) = delete;

    ~VulkanController();

    void DrawFrame();

protected:
    static constexpr uint32_t FRAMES_IN_FLIGHT = 2U;

    struct SwapchainInfo {
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> images = {};
        std::vector<VkSemaphore> canPresent = {};
        VkFormat format = {};
        VkExtent2D extent = {};
    };

    struct CommandBufferInfo {
        std::array<VkCommandPool, FRAMES_IN_FLIGHT> commandPools;
        std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> commandBuffers;
        std::array<VkFence, FRAMES_IN_FLIGHT> commandBufferFences;
        std::array<VkSemaphore, FRAMES_IN_FLIGHT> canRender;
        uint32_t fif = 0U;
    };

    void InitInstance();
#ifdef VULKAN_DEBUG_VALIDATION_LAYERS
    void InitDebugUtilsMessanger();
#endif // VULKAN_DEBUG_VALIDATION_LAYERS
    void InitSurface();
    void InitPhysicalDevice();
    void InitQueueFamilyIndex();
    void InitDevice();
    void InitQueue();
    void InitSwapchain();
    void InitCommandBuffers();

    void RecordCommandBuffer(VkImage swapchainImage, uint32_t fif);

    VkInstance instance = VK_NULL_HANDLE;
#ifdef VULKAN_DEBUG_VALIDATION_LAYERS
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI;
    VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;
#endif // VULKAN_DEBUG_VALIDATION_LAYERS
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0U;
    VkQueue queue = VK_NULL_HANDLE;
    SwapchainInfo swapchainInfo = {};
    CommandBufferInfo commandBufferInfo = {};

    Core core{};
};

}

#include "vulkan_controller.hpp"
#include "utils/window.hpp"
#include "my_vulkan/utils.hpp"
#include "vulkan_functions.hpp"

#include <algorithm>
#include <format>
#include <cstring>
#include <optional>

namespace {

constexpr char const *VK_LAYER_KHRONOS_VALIDATION_NAME = "VK_LAYER_KHRONOS_validation";

#ifdef BLACK_HOLE_RAY_QUERY
constexpr uint32_t VULKAN_API_VERSION = VK_API_VERSION_1_2;
#else
constexpr uint32_t VULKAN_API_VERSION = VK_API_VERSION_1_0;
#endif // BLACK_HOLE_RAY_QUERY

constexpr char const *requiredDeviceExtensions[] = {
#ifdef BLACK_HOLE_RAY_QUERY
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_QUERY_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
#endif // BLACK_HOLE_RAY_QUERY
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

constexpr char const *requiredInstanceLayers[] = {
    VK_LAYER_KHRONOS_VALIDATION_NAME
};

}

namespace KRV {

VulkanController::VulkanController() {
    LoadVulkanGlobalFunctions();
    InitInstance();
    LoadVulkanInstanceFunctions(instance);
#ifdef VULKAN_DEBUG_VALIDATION_LAYERS
    InitDebugUtilsMessanger();
#endif // VULKAN_DEBUG_VALIDATION_LAYERS
    InitSurface();
    InitPhysicalDevice();
    InitQueueFamilyIndex();
    InitDevice();
    LoadVulkanDeviceFunctions(device);
    InitQueue();
    InitSwapchain();
    InitCommandBuffers();

    core.Init(physicalDevice, device);
}

void VulkanController::InitInstance() {
    std::vector<const char*> extensions = Window::GetInstance().GetVulkanSurfaceExtensions();
#if defined(VULKAN_DEBUG_NAMES) || defined(VULKAN_DEBUG_VALIDATION_LAYERS)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#ifdef VULKAN_DEBUG_VALIDATION_LAYERS
    extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
#endif // VULKAN_DEBUG_VALIDATION_LAYERS
#endif // VULKAN_DEBUG_NAMES, VULKAN_DEBUG_VALIDATION_LAYERS

    VkApplicationInfo applicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "ApplicationName",
        .applicationVersion = 1,
        .pEngineName = "EngineName",
        .engineVersion = 1,
        .apiVersion = VULKAN_API_VERSION
    };

    VkInstanceCreateInfo instanceCI {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = 0U,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

#ifdef VULKAN_DEBUG_VALIDATION_LAYERS
    constexpr VkBool32 vkTrue = VK_TRUE;
    constexpr VkBool32 vkFalse = VK_FALSE;
    constexpr char const *messageSeverity[] = {"error"};

    VkLayerSettingEXT layerSettings[] = {
        {
            .pLayerName = VK_LAYER_KHRONOS_VALIDATION_NAME,
            .pSettingName = "syncval_shader_accesses_heuristic",
            .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
            .valueCount = 1U,
            .pValues = &vkTrue
        },
        {
            .pLayerName = VK_LAYER_KHRONOS_VALIDATION_NAME,
            .pSettingName = "validate_sync",
            .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
            .valueCount = 1U,
            .pValues = &vkTrue
        },
        {
            .pLayerName = VK_LAYER_KHRONOS_VALIDATION_NAME,
            .pSettingName = "validate_best_practices",
            .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
            .valueCount = 1U,
            .pValues = &vkTrue
        },
        {
            .pLayerName = VK_LAYER_KHRONOS_VALIDATION_NAME,
            .pSettingName = "report_flags",
            .type = VK_LAYER_SETTING_TYPE_STRING_EXT,
            .valueCount = std::size(messageSeverity),
            .pValues = messageSeverity
        }
    };

    debugUtilsMessengerCI = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0U,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = Utils::DebugUtils::DebugCallback,
        .pUserData = nullptr
    };

    VkLayerSettingsCreateInfoEXT layerSettingsCI {
        .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
        .pNext = &debugUtilsMessengerCI,
        .settingCount = std::size(layerSettings),
        .pSettings = layerSettings
    };

    instanceCI.pNext = &layerSettingsCI;
    instanceCI.enabledLayerCount = std::size(requiredInstanceLayers);
    instanceCI.ppEnabledLayerNames = requiredInstanceLayers;
#endif // VULKAN_DEBUG_VALIDATION_LAYERS

    VK_CALL(vkCreateInstance(&instanceCI, nullptr, &instance));
}

#ifdef VULKAN_DEBUG_VALIDATION_LAYERS
void VulkanController::InitDebugUtilsMessanger() {
    VK_CALL(vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, nullptr, &debugUtilsMessenger));
}
#endif // VULKAN_DEBUG_VALIDATION_LAYERS

void VulkanController::InitSurface() {
    surface = Window::GetInstance().CreateVulkanSurface(instance);
}

void VulkanController::InitPhysicalDevice() {
    uint32_t physicalDeviceCount = 0U;

    VK_CALL(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

    VK_CALL(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

    struct PhysicalDeviceInfo {
        std::optional<uint32_t> discreteIndex = std::nullopt;
        std::optional<uint32_t> integratedIndex = std::nullopt;
    } physicalDeviceInfo;

    for (uint32_t i = 0U; i < physicalDeviceCount; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {physicalDeviceInfo.discreteIndex = i;}
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {physicalDeviceInfo.integratedIndex = i;}
    }

#ifndef VULKAN_IGNORE_DISCRETE_GPU
    if (auto const &idx = physicalDeviceInfo.discreteIndex; idx) {physicalDevice = physicalDevices[*idx]; return;}
#endif // VULKAN_IGNORE_DISCRETE_GPU
    if (auto const &idx = physicalDeviceInfo.integratedIndex; idx) {physicalDevice = physicalDevices[*idx]; return;}

    throw std::runtime_error("There is no discrete or integrated phisical device");
}

void VulkanController::InitQueueFamilyIndex() {

    uint32_t queueFamilyCount = 0U;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    // For every queue family check graphics, compute, transfer and presentation flags.
    constexpr VkQueueFlags requiredQueueFlags = (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
    for (queueFamilyIndex = 0U; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++) {
        if ((queueFamilyProperties[queueFamilyIndex].queueFlags & requiredQueueFlags) == requiredQueueFlags) {
            VkBool32 presentationSupport = VK_FALSE;
            VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &presentationSupport));
            if (presentationSupport == VK_TRUE) {
                return;
            }
        }
    }

    // May be only graphic queue and only compute queue, but I ignore this case, because I am in comfort zone.
    throw std::runtime_error(R"(queueFamilies don't support graphics, comput, transfer and presentation together)");
}

void VulkanController::InitDevice() {
    VkDeviceQueueCreateInfo deviceQueueCI {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1U,
        .pQueuePriorities = &ONE_FLOAT
    };

    ////////////// Physical Device Features Structure //////////////
    VkPhysicalDeviceFeatures physicalDeviceFeatures {
#ifdef BLACK_HOLE_RAY_QUERY
        .shaderInt64 = VK_TRUE
#endif // BLACK_HOLE_RAY_QUERY
    };
    ////////////////////////////////////////////////////////////////

    void *deviceCIpNext = nullptr;

    ///////////////// Device Extensions Structures /////////////////
#ifdef BLACK_HOLE_RAY_QUERY
    VkPhysicalDeviceVulkan12Features vulkan12Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = nullptr,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE
    };

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
        .pNext = &vulkan12Features,
        .rayQuery = VK_TRUE
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext = &rayQueryFeatures,
        .accelerationStructure = VK_TRUE,
        .accelerationStructureCaptureReplay = VK_FALSE,
        .accelerationStructureIndirectBuild = VK_FALSE,
        .accelerationStructureHostCommands = VK_FALSE,
        .descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE
    };

    deviceCIpNext = &asFeatures;
#endif // BLACK_HOLE_RAY_QUERY
    ////////////////////////////////////////////////////////////////

    VkDeviceCreateInfo deviceCI {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = deviceCIpNext,
        .flags = 0U,
        .queueCreateInfoCount = 1U,
        .pQueueCreateInfos = &deviceQueueCI,
        .enabledLayerCount = 0U,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(std::size(requiredDeviceExtensions)),
        .ppEnabledExtensionNames = requiredDeviceExtensions,
        .pEnabledFeatures = &physicalDeviceFeatures
    };

    VK_CALL(vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device));
}

void VulkanController::InitQueue() {
    vkGetDeviceQueue(device, queueFamilyIndex, 0U, &queue);
}

void VulkanController::InitSwapchain() {
    // Get Surface Present Mode
    VkPresentModeKHR presentMode = [&](){
        uint32_t presentModeCount = 0U;
        VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

        struct Checker {
            bool mailbox = false;
            bool immediate = false;
        } checker;

        std::ranges::for_each(presentModes, [&checker](VkPresentModeKHR presentMode){
            switch (presentMode) {
                case VK_PRESENT_MODE_MAILBOX_KHR:
                    checker.mailbox = true;
                    break;
                case VK_PRESENT_MODE_IMMEDIATE_KHR:
                    checker.immediate = true;
                    break;
                default:
                    // Nothing
                    break;
            }
        });

        if (checker.mailbox) {return VK_PRESENT_MODE_MAILBOX_KHR;}
        if (checker.immediate) {return VK_PRESENT_MODE_IMMEDIATE_KHR;}
        return VK_PRESENT_MODE_FIFO_KHR; // Always present
    }();

    // Get Surface formats
    uint32_t formatCount = 0U;
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()));

    // Get Surface Capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities));

    swapchainInfo.extent = [&](){
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = Window::GetInstance().GetWindowSize();
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            return actualExtent;
        }
    }();
    swapchainInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    VkSwapchainKHR& swapchain = swapchainInfo.swapchain;
    auto& swapchainImages = swapchainInfo.images;

    // TODO: Make it cross-device
    VkSwapchainCreateInfoKHR swapchainCI {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0U,
        .surface = surface,
        .minImageCount = std::clamp(capabilities.minImageCount + 1U, capabilities.minImageCount, capabilities.maxImageCount),
        .imageFormat = swapchainInfo.format,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = swapchainInfo.extent,
        .imageArrayLayers = 1U,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1U,
        .pQueueFamilyIndices = &queueFamilyIndex,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    VK_CALL(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));

    uint32_t swapchainImageCount = 0U;
    VK_CALL(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr));
    swapchainImages.resize(swapchainImageCount);
    VK_CALL(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()));

    swapchainInfo.canPresent.resize(swapchainImageCount);
    constexpr VkSemaphoreCreateInfo semaphoreCI {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U
    };

    for (uint32_t i = 0U; i < swapchainImageCount; i++) {
        VkSemaphore &canPresent = swapchainInfo.canPresent[i];
        VK_CALL(vkCreateSemaphore(device, &semaphoreCI, nullptr, &canPresent));
        Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_SEMAPHORE, canPresent, std::format("Queue CanPresent Semaphore [{}]", i).c_str());
    }
}


void VulkanController::InitCommandBuffers() {
    constexpr VkFenceCreateInfo fenceCI {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0U; i < FRAMES_IN_FLIGHT; i++) {
        VkCommandPool& commandPool = commandBufferInfo.commandPools[i];

        VkCommandPoolCreateInfo commandPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = queueFamilyIndex
        };

        VK_CALL(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));

        VkCommandBufferAllocateInfo commandBufferAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1U
        };

        VkCommandBuffer &commandBuffer = commandBufferInfo.commandBuffers[i];

        VK_CALL(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer));

        Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, commandBuffer, std::format("Command Buffer [{}]", i).c_str());

        VkFence& fence = commandBufferInfo.commandBufferFences[i];
        VK_CALL(vkCreateFence(device, &fenceCI, nullptr, &fence));
        Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_FENCE, fence, std::format("Command Buffer Fence [{}]", i).c_str());

        constexpr VkSemaphoreCreateInfo semaphoreCI {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0U
        };

        VkSemaphore& canRender = commandBufferInfo.canRender[i];
        VK_CALL(vkCreateSemaphore(device, &semaphoreCI, nullptr, &canRender));
        Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_SEMAPHORE, canRender, std::format("Queue CanRender Semaphore [{}]", i).c_str());
    }
}

void VulkanController::RecordCommandBuffer(VkImage swapchainImage, uint32_t fif) {
    VkCommandPool commandPool = commandBufferInfo.commandPools[fif];
    VkCommandBuffer commandBuffer = commandBufferInfo.commandBuffers[fif];

    VK_CALL(vkResetCommandPool(device, commandPool, 0U));

    constexpr VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    VK_CALL(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

    {
        Utils::DebugUtils::LabelGuard labelGeneralGuard(commandBuffer, "RecordCommandBuffer", 0.7F, 0.7F, 0.7F);

        Image& finalImage = core.RecordCommandBuffer(device, commandBuffer);

        {
            Utils::DebugUtils::LabelGuard labelBlitGuard(commandBuffer, "Final Blit", 1.0F, 1.0F, 1.0F);

            VkImageMemoryBarrier const firstImageMemoryBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = 0U,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = swapchainImage,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0U,
                    .levelCount = 1U,
                    .baseArrayLayer = 0U,
                    .layerCount = 1U
                }
            };

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0U, nullptr, 0U, nullptr, 1U, &firstImageMemoryBarrier);

            VkImageBlit const region = VkImageBlit{
                .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0U,
                    .baseArrayLayer = 0U,
                    .layerCount = 1U
                },
                .srcOffsets = {
                    {
                        .x = 0,
                        .y = 0,
                        .z = 0
                    },
                    {
                        .x = static_cast<int32_t>(WINDOW_SIZE_WIDTH),
                        .y = static_cast<int32_t>(WINDOW_SIZE_HEIGHT),
                        .z = 1
                    }
                },
                .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0U,
                    .baseArrayLayer = 0U,
                    .layerCount = 1U
                },
                .dstOffsets = {
                    {
                        .x = 0,
                        .y = 0,
                        .z = 0
                    },
                    {
                        .x = static_cast<int32_t>(swapchainInfo.extent.width),
                        .y = static_cast<int32_t>(swapchainInfo.extent.height),
                        .z = 1
                    }
                }
            };

            vkCmdBlitImage(commandBuffer, finalImage.image, finalImage.layout, swapchainImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &region, VK_FILTER_LINEAR);

            VkImageMemoryBarrier const secondImageMemoryBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = 0U,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = swapchainImage,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0U,
                    .levelCount = 1U,
                    .baseArrayLayer = 0U,
                    .layerCount = 1U
                }
            };

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0U, nullptr, 0U, nullptr, 1U, &secondImageMemoryBarrier);
        }
    }

    VK_CALL(vkEndCommandBuffer(commandBuffer));
}

void VulkanController::DrawFrame() {
    uint32_t &fif = commandBufferInfo.fif;
    VK_CALL(vkWaitForFences(device, 1, &commandBufferInfo.commandBufferFences[fif], VK_TRUE, UINT64_MAX));
    VK_CALL(vkResetFences(device, 1, &commandBufferInfo.commandBufferFences[fif]));

    uint32_t imageIndex = 0U;
    VK_CALL(vkAcquireNextImageKHR(device, swapchainInfo.swapchain, UINT64_MAX, commandBufferInfo.canRender[fif], VK_NULL_HANDLE, &imageIndex));

    RecordCommandBuffer(swapchainInfo.images[imageIndex], fif);

    constexpr VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1U,
        .pWaitSemaphores = &commandBufferInfo.canRender[fif],
        .pWaitDstStageMask = &waitDstStageMask,
        .commandBufferCount = 1U,
        .pCommandBuffers = &commandBufferInfo.commandBuffers[fif],
        .signalSemaphoreCount = 1U,
        .pSignalSemaphores = &swapchainInfo.canPresent[imageIndex]
    };

    VK_CALL(vkQueueSubmit(queue, 1U, &submitInfo, commandBufferInfo.commandBufferFences[fif]));

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1U,
        .pWaitSemaphores = &swapchainInfo.canPresent[imageIndex],
        .swapchainCount = 1U,
        .pSwapchains = &swapchainInfo.swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr
    };

    VK_CALL(vkQueuePresentKHR(queue, &presentInfo));

    fif = ++fif % FRAMES_IN_FLIGHT;
}

VulkanController::~VulkanController() {
    // Wait device, before termination
    // No check return value, because we want to terminate Vulkan
    vkDeviceWaitIdle(device);

    core.Destroy(device);

    for (uint32_t i = 0U; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyCommandPool(device, commandBufferInfo.commandPools[i], nullptr);
        vkDestroyFence(device, commandBufferInfo.commandBufferFences[i], nullptr);
        vkDestroySemaphore(device, commandBufferInfo.canRender[i], nullptr);
    }

    for (VkSemaphore &canPresent : swapchainInfo.canPresent) {
        vkDestroySemaphore(device, canPresent, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchainInfo.swapchain, nullptr);
    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

#ifdef VULKAN_DEBUG_VALIDATION_LAYERS
    vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
#endif // VULKAN_DEBUG_VALIDATION_LAYERS

    vkDestroyInstance(instance, nullptr);
}

}

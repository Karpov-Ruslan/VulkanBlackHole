#pragma once

#include <constants.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace KRV {

struct Image {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags stage = 0;
    VkAccessFlags access = 0;
    VkExtent3D size = {
        .width = 0U,
        .height = 0U,
        .depth = 0U
    };
};

struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    VkDeviceSize deviceMemoryOffset = 0ULL;
    VkDeviceSize size = 0ULL;
};

namespace Utils {

class DebugUtils final {
public:
    class LabelGuard final {
    public:
        LabelGuard(VkCommandBuffer commandBuffer, char const *name, float r, float g, float b);

        LabelGuard(LabelGuard const &) = delete;
        LabelGuard& operator=(LabelGuard const &) = delete;
        LabelGuard(LabelGuard &&) = delete;
        LabelGuard& operator=(LabelGuard &&) = delete;

        ~LabelGuard();

    private:
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    };

    static void Name(VkDevice device, VkObjectType objectType, auto object, const char* name) {
#ifdef VULKAN_DEBUG_NAMES
        VkDebugUtilsObjectNameInfoEXT objectNameInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = objectType,
            .objectHandle = reinterpret_cast<uint64_t>(object),
            .pObjectName = name
        };

        NameImpl(device, objectNameInfo);
#endif // VULKAN_DEBUG_NAMES
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData, void* pUserData);

private:
    static void NameImpl(VkDevice device, VkDebugUtilsObjectNameInfoEXT const &objectNameInfo);
};

void CopyMemoryIntoStagingBuffer(VkDevice device, Buffer &stagingBuffer, void *data, VkDeviceSize size);

void MemoryPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStage, VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess);

void ImageChangeProperties(Image& image, VkImageLayout newLayout, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess);

void ImagePipelineBarrier(VkCommandBuffer commandBuffer, Image &target, VkImageLayout dstLayout, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess, VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U});

}

}
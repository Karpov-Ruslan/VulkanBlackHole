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

template <typename T>
void DebugUtilsName(VkDevice device, VkObjectType objectType, T object, const char* name) {
#ifdef VULKAN_DEBUG
    VkDebugUtilsObjectNameInfoEXT objectNameInfo {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = objectType,
        .objectHandle = reinterpret_cast<uint64_t>(object),
        .pObjectName = name
    };

    VK_CALL(vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo));
#endif
}

void MemoryPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStage, VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess);

void ImageChangeProperties(Image& image, VkImageLayout newLayout, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess);

void ImagePipelineBarrier(VkCommandBuffer commandBuffer, Image &target, VkImageLayout dstLayout, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess, VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U});

}

}
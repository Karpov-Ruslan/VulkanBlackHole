#include "utils.hpp"
#include "my_vulkan/vulkan_functions.hpp"

namespace KRV::Utils {

DebugUtils::LabelGuard::LabelGuard(VkCommandBuffer commandBuffer, char const *name,
    float r, float g, float b) : commandBuffer(commandBuffer) {
#ifdef VULKAN_DEBUG
    VkDebugUtilsLabelEXT label {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = name,
        .color = {r, g, b, 1.0F}
    };

    vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
#endif
}

DebugUtils::LabelGuard::~LabelGuard() {
#ifdef VULKAN_DEBUG
    vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
}

void DebugUtils::NameImpl(VkDevice device, VkDebugUtilsObjectNameInfoEXT const &objectNameInfo) {
#ifdef VULKAN_DEBUG
    VK_CALL(vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo));
#endif
}

void ImageChangeProperties(Image& image, VkImageLayout newLayout, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess) {
    image.layout = newLayout;
    image.stage = dstStage;
    image.access = dstAccess;
}

void MemoryPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStage, VkAccessFlags srcAccess, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess) {
    VkMemoryBarrier memoryBarrier {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = srcAccess,
        .dstAccessMask = dstAccess
    };

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, VK_DEPENDENCY_BY_REGION_BIT, 1U, &memoryBarrier, 0U, nullptr, 0U, nullptr);
}

void ImagePipelineBarrier(VkCommandBuffer commandBuffer, Image &target, VkImageLayout dstLayout, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess, VkImageSubresourceRange subresourceRange) {
    VkImageMemoryBarrier imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = target.access,
        .dstAccessMask = dstAccess,
        .oldLayout = target.layout,
        .newLayout = dstLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = target.image,
        .subresourceRange = subresourceRange
    };

    vkCmdPipelineBarrier(commandBuffer, target.stage, dstStage, VK_DEPENDENCY_BY_REGION_BIT, 0U, nullptr, 0U, nullptr, 1U, &imageMemoryBarrier);

    ImageChangeProperties(target, dstLayout, dstStage, dstAccess);
}

}
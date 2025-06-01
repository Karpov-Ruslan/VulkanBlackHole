#include "utils.hpp"
#include "my_vulkan/vulkan_functions.hpp"

#include <iostream>

namespace KRV::Utils {

DebugUtils::LabelGuard::LabelGuard(VkCommandBuffer commandBuffer, char const *name,
    float r, float g, float b) : commandBuffer(commandBuffer) {
#if defined(VULKAN_DEBUG_NAMES) || defined (VULKAN_DEBUG_VALIDATION_LAYERS)
    VkDebugUtilsLabelEXT label {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = name,
        .color = {r, g, b, 1.0F}
    };

    vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &label);
#endif // VULKAN_DEBUG_NAMES, VULKAN_DEBUG_VALIDATION_LAYERS
}

DebugUtils::LabelGuard::~LabelGuard() {
#if defined(VULKAN_DEBUG_NAMES) || defined (VULKAN_DEBUG_VALIDATION_LAYERS)
    vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif // VULKAN_DEBUG_NAMES, VULKAN_DEBUG_VALIDATION_LAYERS
}

void DebugUtils::NameImpl(VkDevice device, VkDebugUtilsObjectNameInfoEXT const &objectNameInfo) {
#if defined(VULKAN_DEBUG_NAMES) || defined (VULKAN_DEBUG_VALIDATION_LAYERS)
    VK_CALL(vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo));
#endif // VULKAN_DEBUG_NAMES, VULKAN_DEBUG_VALIDATION_LAYERS
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtils::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData, void* pUserData) {

    static auto const printVVL = [](VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData){
        std::cout << "\n\tObjects: ";
        for (uint32_t i = 0U; i < pCallbackData->objectCount; i++) {
            auto const &objectName = pCallbackData->pObjects[i].pObjectName;
            if (objectName) {
                std::cout << objectName << " ";
            } else {
                std::cout << pCallbackData->pObjects[i].objectHandle << " ";
            }
        }

        auto pMessageIdName = pCallbackData->pMessageIdName ? pCallbackData->pMessageIdName : "NO_MESSAGE_ID_NAME";
        auto pMessage = pCallbackData->pMessage ? pCallbackData->pMessage : "NO_MESSAGE";
        std::cout << std::format("\n\t[ {} ] | MessageID = {}\n\t{}", pMessageIdName, pCallbackData->messageIdNumber, pMessage);
    };

    // Use ANSI color codes
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cout << "\033[33m" << std::endl;
        std::cout << "Validation Warning: ";
        printVVL(pCallbackData);
        std::cout << "\033[39m" << std::endl;
    } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cout << "\033[31m" << std::endl;
        std::cout << "Validation Error: ";
        printVVL(pCallbackData);
        std::cout << "\033[39m" << std::endl;
    }

    return VK_FALSE;
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
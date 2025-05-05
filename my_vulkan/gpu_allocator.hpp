#pragma once

#include <my_vulkan/utils.hpp>
#include <unordered_map>
#include <functional>
#include <string_view>

namespace KRV::Utils {

struct CreateImageInfo {
    VkImageType type = VK_IMAGE_TYPE_2D;
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    VkFlags flags = 0U;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent3D extent = {}; // Must be set up
    uint32_t mipLayers = 1U;
    uint32_t arrayLayers = 1U;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageUsageFlags usage = 0U; // Must be set up
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    VkComponentMapping components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY
    };
    std::string name = ""; // Should be set
};

struct CreateBufferInfo {
    VkDeviceSize size = 0U;
    VkBufferUsageFlags usage = 0U;
    std::string name = "";
};

class GPUAllocator {
public:
    GPUAllocator() = default;

    GPUAllocator(const GPUAllocator&) = delete;
    GPUAllocator& operator=(const GPUAllocator&) = delete;
    GPUAllocator(GPUAllocator&&) = delete;
    GPUAllocator& operator=(GPUAllocator&&) = delete;

    void Init(VkPhysicalDevice physicalDevice);

    void Destroy(VkDevice device);

    // Mark up memory for an image and give index of Image
    Image& AddImage(VkDevice device, CreateImageInfo const &createImageInfo,
        VkMemoryPropertyFlags requiredMemoryFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VkMemoryPropertyFlags avoidableMemoryFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    // Mark up memory for a buffer and give index of Buffer
    Buffer& AddBuffer(VkDevice device, CreateBufferInfo const &createBufferInfo,
        VkMemoryPropertyFlags requiredMemoryFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VkMemoryPropertyFlags avoidableMemoryFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    Image& GetImage(std::string_view const &name);
    Buffer& GetBuffer(std::string_view const &name);

    // Bind memory with all marked objects
    void PresentResources(VkDevice device);

private:
    struct ImageInfo {
        Image image;
        CreateImageInfo createImageInfo;
        uint32_t memoryTypeIndex = 0U;
        VkDeviceSize requiredOffset = 0ULL;
    };

    struct BufferInfo {
        Buffer buffer;
        CreateBufferInfo createBufferInfo;
        uint32_t memoryTypeIndex = 0U;
        VkDeviceSize requiredOffset = 0ULL;
    };

    void BindImageMemory(VkDevice device);

    void BindBufferMemory(VkDevice device);

    // Image queue
    std::vector<ImageInfo> imageInfos = {};
    std::unordered_map<std::string_view, Image*> imageMapper;

    // Buffer queue
    std::vector<BufferInfo> bufferInfos = {};
    std::unordered_map<std::string_view, Buffer*> bufferMapper;

    // Memory
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    VkDeviceMemory deviceMemory[VK_MAX_MEMORY_TYPES] = {};
    VkDeviceSize memorySize[VK_MAX_MEMORY_TYPES] = {};
};

}
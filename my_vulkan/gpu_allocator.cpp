#include "gpu_allocator.hpp"
#include "my_vulkan/vulkan_functions.hpp"

#include <bitset>

namespace {

void CreateImage(VkDevice device, const KRV::Utils::CreateImageInfo& createImageInfo, KRV::Image& image) {
    VkImageCreateInfo imageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = createImageInfo.flags,
        .imageType = createImageInfo.type,
        .format = createImageInfo.format,
        .extent = createImageInfo.extent,
        .mipLevels = createImageInfo.mipLayers,
        .arrayLayers = createImageInfo.arrayLayers,
        .samples = createImageInfo.samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = createImageInfo.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0U,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = createImageInfo.initialLayout
    };

    VK_CALL(vkCreateImage(device, &imageCreateInfo, nullptr, &image.image));

    KRV::Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_IMAGE, image.image, createImageInfo.name.c_str());
}

void CreateImageView(VkDevice device, const KRV::Utils::CreateImageInfo& createImageInfo, KRV::Image& image) {
    VkImageViewCreateInfo viewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .image = image.image,
        .viewType = createImageInfo.viewType,
        .format = createImageInfo.format,
        .components = createImageInfo.components,
        .subresourceRange = {
            .aspectMask = createImageInfo.aspect,
            .baseMipLevel = 0U,
            .levelCount = createImageInfo.mipLayers,
            .baseArrayLayer = 0U,
            .layerCount = createImageInfo.arrayLayers
        }
    };

    VK_CALL(vkCreateImageView(device, &viewCreateInfo, nullptr, &image.imageView));

    KRV::Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_IMAGE_VIEW, image.imageView, createImageInfo.name.c_str());

    image.layout = createImageInfo.initialLayout;
    image.access = 0U;
    image.stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    image.size = createImageInfo.extent;
}

void CreateBuffer(VkDevice device, const KRV::Utils::CreateBufferInfo& createBufferInfo, KRV::Buffer& buffer) {
    VkBufferCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .size = createBufferInfo.size,
        .usage = createBufferInfo.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0U,
        .pQueueFamilyIndices = nullptr
    };

    VK_CALL(vkCreateBuffer(device, &createInfo, nullptr, &buffer.buffer));

    KRV::Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_BUFFER, buffer.buffer, createBufferInfo.name.c_str());

    buffer.size = createBufferInfo.size;
}

}

namespace KRV::Utils {

void GPUAllocator::BindImageMemory(VkDevice device) {
    for (auto& it : imageInfos) {
        VK_CALL(vkBindImageMemory(device, it.image.image, deviceMemory[it.memoryTypeIndex], it.requiredOffset));
    }
}

void GPUAllocator::BindBufferMemory(VkDevice device) {
    for (auto& it : bufferInfos) {
        VkDeviceMemory deviceMem = deviceMemory[it.memoryTypeIndex];
        auto &buffer = it.buffer;
        VK_CALL(vkBindBufferMemory(device, buffer.buffer, deviceMem, it.requiredOffset));
        buffer.deviceMemory = deviceMem;
        buffer.deviceMemoryOffset = it.requiredOffset;
    }
}

void GPUAllocator::Init(VkPhysicalDevice physicalDevice) {
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
}

void GPUAllocator::Destroy(VkDevice device) {
    // Destroy Images and its ImageViews
    for (auto& it : imageInfos) {
        vkDestroyImageView(device, it.image.imageView, nullptr);
        vkDestroyImage(device, it.image.image, nullptr);
    }
    imageInfos.clear();

    // Destroy Buffers
    for (auto& it : bufferInfos) {
        vkDestroyBuffer(device, it.buffer.buffer, nullptr);
    }
    bufferInfos.clear();

    // Free Memory
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        vkFreeMemory(device, deviceMemory[i], nullptr);
    }
}

Image& GPUAllocator::AddImage(VkDevice device, const CreateImageInfo& createImageInfo,
    VkMemoryPropertyFlags requiredMemoryFlag, VkMemoryPropertyFlags avoidableMemoryFlag) {
    Image image{};
    CreateImage(device, createImageInfo, image);

    VkMemoryRequirements memoryRequirements = {};

    vkGetImageMemoryRequirements(device, image.image, &memoryRequirements);

    const std::bitset<8*sizeof(uint32_t)> memoryTypeBitsRepr(memoryRequirements.memoryTypeBits);

    for (size_t i = 0; i < memoryTypeBitsRepr.size(); i++) {
        if (memoryTypeBitsRepr[i] == true) {
            if ((memoryProperties.memoryTypes[i].propertyFlags & requiredMemoryFlag) == requiredMemoryFlag) {
                if ((memoryProperties.memoryTypes[i].propertyFlags & avoidableMemoryFlag) == 0U) {
                    const auto& alignment = memoryRequirements.alignment;
                    const VkDeviceSize memoryOffset = ((memorySize[i] + alignment - 1ULL) & (~(alignment - 1ULL)));

                    memorySize[i] = memoryOffset + memoryRequirements.size;
                    imageInfos.emplace_back(image, createImageInfo, i, memoryOffset);
                    imageMapper[createImageInfo.name] = &imageInfos.back().image;
                    return imageInfos.back().image;
                }
            }
        }
    }

    throw std::runtime_error("GPUAllocator : Memory Type was not found");
}

Buffer& GPUAllocator::AddBuffer(VkDevice device, const CreateBufferInfo& createBufferInfo,
    VkMemoryPropertyFlags requiredMemoryFlag, VkMemoryPropertyFlags avoidableMemoryFlag) {
    Buffer buffer{};
    CreateBuffer(device, createBufferInfo, buffer);

    VkMemoryRequirements memoryRequirements = {};

    vkGetBufferMemoryRequirements(device, buffer.buffer, &memoryRequirements);

    const std::bitset<8*sizeof(uint32_t)> memoryTypeBitsRepr(memoryRequirements.memoryTypeBits);

    for (size_t i = 0; i < memoryTypeBitsRepr.size(); i++) {
        if (memoryTypeBitsRepr[i] == true) {
            if ((memoryProperties.memoryTypes[i].propertyFlags & requiredMemoryFlag) == requiredMemoryFlag) {
                if ((memoryProperties.memoryTypes[i].propertyFlags & avoidableMemoryFlag) == 0U) {
                    const auto& alignment = memoryRequirements.alignment;
                    const VkDeviceSize memoryOffset = ((memorySize[i] + alignment - 1ULL) & (~(alignment - 1ULL)));

                    memorySize[i] = memoryOffset + memoryRequirements.size;
                    bufferInfos.emplace_back(buffer, createBufferInfo, i, memoryOffset);
                    bufferMapper[createBufferInfo.name] = &bufferInfos.back().buffer;
                    return bufferInfos.back().buffer;
                }
            }
        }
    }

    throw std::runtime_error("GPUAllocator : Memory Type was not found");
}

Image& GPUAllocator::GetImage(std::string_view const &name) {
    return *imageMapper[name];
}

Buffer& GPUAllocator::GetBuffer(std::string_view const &name) {
    return *bufferMapper[name];
}

void GPUAllocator::PresentResources(VkDevice device) {
    // Allocate Memory
    for (uint32_t i = 0U; i < VK_MAX_MEMORY_TYPES; i++) {
        if (memorySize[i] != 0ULL) {
            VkMemoryAllocateInfo memoryAllocateInfo {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = memorySize[i],
                .memoryTypeIndex = i
            };

            VK_CALL(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory[i]));
        }
    }

    // Bind Memory
    BindImageMemory(device);
    BindBufferMemory(device);

    // Finish creating of images
    for (auto& it : imageInfos) {
        CreateImageView(device, it.createImageInfo, it.image);
    }
}


}
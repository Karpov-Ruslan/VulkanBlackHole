#include "black_hole_pass.hpp"

#include "my_vulkan/shaders/shaders_list.hpp"
#include "my_vulkan/shaders/black_hole.in"

#include "my_vulkan/vulkan_functions.hpp"

#include <cstring>

// TODO: Move it into another file
#define STB_IMAGE_IMPLEMENTATION
#include "third-party/stb_image.h"

namespace {
    constexpr char const *cubeMapsFaceNames[6] = {
        "textures/black_hole/front.png",
        "textures/black_hole/back.png",
        "textures/black_hole/top.png",
        "textures/black_hole/bottom.png",
        "textures/black_hole/left.png",
        "textures/black_hole/right.png"
    };

    constexpr uint32_t cubeMapFacesNum = std::size(cubeMapsFaceNames);
}

namespace KRV {

void BlackHolePass::AllocateResources(VkDevice device, Utils::GPUAllocator& gpuAllocator) {
    Utils::CreateImageInfo createImageInfo {
        .extent = {
            .width = WINDOW_SIZE_WIDTH,
            .height = WINDOW_SIZE_HEIGHT,
            .depth = 1U
        },
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .name = "BlackHolePass::finalImage"
    };

    pFinalImage = &gpuAllocator.AddImage(device, createImageInfo);

    AllocateCubeMap(device, gpuAllocator);
}

void BlackHolePass::Init(VkDevice device) {
    InitSampler(device);
    InitDescriptorSet(device);
    InitPipeline(device);
}

void BlackHolePass::Destroy(VkDevice device) {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroySampler(device, sampler, nullptr);
}

void BlackHolePass::RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) {
    Utils::DebugUtils::LabelGuard labelGuard(commandBuffer, "BlackHolePass", 0.5F, 0.0F, 0.0F);

    if (!cubeMapIsLoaded) {
        LoadCubeMap(device, commandBuffer);
        cubeMapIsLoaded = true;
    }

    // Force to undefined image layout, because of performance
    pFinalImage->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    Utils::ImagePipelineBarrier(commandBuffer, *pFinalImage, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);

    // Update Push Constants
    camera.Update();
    auto const &position = camera.GetPosition();
    auto const &direction = camera.GetDirection();
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        PUSH_CONSTANT_CAMERA_POSITION_OFFSET, 3U*sizeof(float), &position);
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        PUSH_CONSTANT_CAMERA_DIRECTION_OFFSET, 3U*sizeof(float), &direction);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0U, 1U, &descriptorSet, 0U, nullptr);
    vkCmdDispatch(commandBuffer, WINDOW_SIZE_WIDTH/LOCAL_SIZE_X, WINDOW_SIZE_HEIGHT/LOCAL_SIZE_Y, 1U);

    Utils::ImagePipelineBarrier(commandBuffer, *pFinalImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
}

void BlackHolePass::InitSampler(VkDevice device) {
    VkSamplerCreateInfo samplerCI {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .mipLodBias = 0.0F,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0F,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0F,
        .maxLod = 0.0F,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    VK_CALL(vkCreateSampler(device, &samplerCI, nullptr, &sampler));

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_SAMPLER, sampler, "BlackHolePass::Sampler");
}

void BlackHolePass::InitDescriptorSet(VkDevice device) {
    VkDescriptorPoolSize descriptorPoolSize {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1U
    };

    VkDescriptorPoolCreateInfo descriptorPoolCI {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .maxSets = 1U,
        .poolSizeCount = 1U,
        .pPoolSizes = &descriptorPoolSize
    };

    VK_CALL(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, descriptorPool, "BlackHolePass::DescriptorPool");

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
        {
            .binding = BINDING_FINAL_IMAGE,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1U,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        {
            .binding = BINDING_CUBE_MAP,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1U,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = &sampler
        }
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .bindingCount = std::size(descriptorSetLayoutBindings),
        .pBindings = descriptorSetLayoutBindings
    };

    VK_CALL(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout))

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, descriptorSetLayout, "BlackHolePass::DescriptorSetLayout");

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1U,
        .pSetLayouts = &descriptorSetLayout
    };

    VK_CALL(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet))

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, descriptorSet, "BlackHolePass::DescriptorSet");

    VkDescriptorImageInfo descriptorImageInfo[] = {
        // Final Image
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = pFinalImage->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        },
        // CubeMap
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = pCubeMap->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
    };

    VkWriteDescriptorSet writeDescriptors[] = {
        // Final Image
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSet,
            .dstBinding = BINDING_FINAL_IMAGE,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &descriptorImageInfo[0],
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        },
        // CubeMap
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSet,
            .dstBinding = BINDING_CUBE_MAP,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &descriptorImageInfo[1],
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        }
    };

    vkUpdateDescriptorSets(device, std::size(writeDescriptors), writeDescriptors, 0U, nullptr);
}

void BlackHolePass::InitPipeline(VkDevice device) {
    VkPushConstantRange pushConstantRanges[] = {
        {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = PUSH_CONSTANT_CAMERA_POSITION_OFFSET,
            .size = PUSH_CONSTANT_CAMERA_DIRECTION_OFFSET + 3U*sizeof(float)
        }
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .setLayoutCount = 1U,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = std::size(pushConstantRanges),
        .pPushConstantRanges = pushConstantRanges
    };

    VK_CALL(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, pipelineLayout, "BlackHolePass::PipelineLayout");

    VkShaderModule blackHoleComp = Utils::GetShaderModule(device, Utils::SHADER_LIST_ID::BLACK_HOLE_COMP);

    VkPipelineShaderStageCreateInfo stageCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = blackHoleComp,
        .pName = "main",
        .pSpecializationInfo = nullptr
    };

    VkComputePipelineCreateInfo pipelineCI {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .stage = stageCI,
        .layout = pipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0U
    };

    VK_CALL(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1U, &pipelineCI, nullptr, &pipeline));

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_PIPELINE, pipeline, "BlackHolePass::Pipeline");

    vkDestroyShaderModule(device, blackHoleComp, nullptr);
}

Image& BlackHolePass::GetFinalImage() {
    return *pFinalImage;
}

void BlackHolePass::AllocateCubeMap(VkDevice device, Utils::GPUAllocator& gpuAllocator) {
    int isize_x, isize_y;
    stbi_info(cubeMapsFaceNames[0], &isize_x, &isize_y, nullptr);
    uint32_t size_x = isize_x, size_y = isize_y;

    Utils::CreateBufferInfo stagingBufferCI {
        .size = cubeMapFacesNum*size_x*size_y*4U*sizeof(uint8_t),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .name = "BlackHolePass::StagingBuffer"
    };

    pStagingBuffer = &gpuAllocator.AddBuffer(device, stagingBufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 0U);

    Utils::CreateImageInfo cubeMapCI {
        .type = VK_IMAGE_TYPE_2D,
        .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
        .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .extent = {
            .width = size_x,
            .height = size_y,
            .depth = 1U
        },
        .arrayLayers = cubeMapFacesNum,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .name = "BlackHolePass::CubeMap"
    };

    pCubeMap = &gpuAllocator.AddImage(device, cubeMapCI);
}

void BlackHolePass::LoadCubeMap(VkDevice device, VkCommandBuffer commandBuffer) {
    Utils::DebugUtils::LabelGuard loadCubeMapGuard(commandBuffer, "BlackHolePass::LoadCubeMap", 0.0F, 1.0F, 0.0F);

    // Tricky barriers: They are executed after map operation, because we just write commands into commandBuffer
    Utils::MemoryPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_WRITE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    Utils::ImagePipelineBarrier(commandBuffer, *pCubeMap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, cubeMapFacesNum});

    std::vector<uint8_t> mappedStagingBuffer(pStagingBuffer->size);
    void *pMappedStagingBuffer = mappedStagingBuffer.data();
    VK_CALL(vkMapMemory(device, pStagingBuffer->deviceMemory, pStagingBuffer->deviceMemoryOffset, pStagingBuffer->size, 0U, &pMappedStagingBuffer));

    auto f = [&](uint32_t faceIndex){
        const uint32_t size_x = pCubeMap->size.width, size_y = pCubeMap->size.height;
        int x = size_x, y = size_y, channels = 4U;
        uint8_t *copyData = stbi_load(cubeMapsFaceNames[faceIndex], &x, &y, &channels, 0U);

        VkDeviceSize const faceSize = pStagingBuffer->size/cubeMapFacesNum;
        std::memcpy(static_cast<uint8_t*>(pMappedStagingBuffer) + faceSize*static_cast<VkDeviceSize>(faceIndex), copyData, faceSize);

        VkDeviceSize bufferOffset = faceSize*static_cast<VkDeviceSize>(faceIndex);
        VkMappedMemoryRange mappedMemoryRange {
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext = nullptr,
            .memory = pStagingBuffer->deviceMemory,
            .offset = pStagingBuffer->deviceMemoryOffset + bufferOffset,
            .size = faceSize
        };

        VK_CALL(vkFlushMappedMemoryRanges(device, 1U, &mappedMemoryRange));

        stbi_image_free(copyData);

        // Copy buffer data to image data
        VkBufferImageCopy bufferImageCopy {
            .bufferOffset = bufferOffset,
            .bufferRowLength = 0U,
            .bufferImageHeight = 0U,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0U,
                .baseArrayLayer = faceIndex,
                .layerCount = 1U
            },
            .imageOffset = {
                .x = 0,
                .y = 0,
                .z = 0
            },
            .imageExtent = {
                .width = size_x,
                .height = size_y,
                .depth = 1U
            }
        };

        vkCmdCopyBufferToImage(commandBuffer, pStagingBuffer->buffer, pCubeMap->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &bufferImageCopy);
    };

    for (uint32_t faceIndex = 0U; faceIndex < cubeMapFacesNum; faceIndex++) {
        f(faceIndex);
    }

    vkUnmapMemory(device, pStagingBuffer->deviceMemory);

    Utils::ImagePipelineBarrier(commandBuffer, *pCubeMap, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, {VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, cubeMapFacesNum});
}


}

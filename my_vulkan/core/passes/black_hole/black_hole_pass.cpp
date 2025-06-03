#include "black_hole_pass.hpp"

#include "my_vulkan/shaders/shaders_list.hpp"
#include "my_vulkan/shaders/black_hole.in"
#include "obj_transformation_matrices.hpp"

#include "my_vulkan/vulkan_functions.hpp"

#include "common.hpp"

#include <cstring>
#include <cmath>
#include <utility>

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

#ifdef BLACK_HOLE_PRECOMPUTED
    pPrecomputedPhiTexture = &gpuAllocator.GetImage(PRECOMPUTED_PHI_TEXTURE_NAME);
    pPrecomputedAccrDiskDataTexture = &gpuAllocator.GetImage(PRECOMPUTED_ACCR_DISK_DATA_TEXTURE_NAME);
#endif // BLACK_HOLE_PRECOMPUTED

#ifdef BLACK_HOLE_RAY_QUERY
    AllocateBottomLevelASes(device, gpuAllocator, std::size(blasTransformMatrices));
    AllocateTopLevelAS(device, gpuAllocator);
#endif // BLACK_HOLE_RAY_QUERY
}

void BlackHolePass::Init(VkDevice device) {
    InitSampler(device);
    InitDescriptorSet(device);
    InitPipeline(device);
}

void BlackHolePass::Destroy(VkDevice device) {

#ifdef BLACK_HOLE_RAY_QUERY
    vkDestroyAccelerationStructureKHR(device, std::exchange(tlasInfo.tlas, VK_NULL_HANDLE), nullptr);

    for (auto &blasInfo : blasInfos) {
        vkDestroyAccelerationStructureKHR(device, std::exchange(blasInfo.blas, VK_NULL_HANDLE), nullptr);
    }
#endif // BLACK_HOLE_RAY_QUERY

    vkDestroyPipeline(device, std::exchange(pipeline, VK_NULL_HANDLE), nullptr);
    vkDestroyPipelineLayout(device, std::exchange(pipelineLayout, VK_NULL_HANDLE), nullptr);
    vkDestroyDescriptorSetLayout(device, std::exchange(descriptorSetLayout, VK_NULL_HANDLE), nullptr);
    vkDestroyDescriptorPool(device, std::exchange(descriptorPool, VK_NULL_HANDLE), nullptr);
    vkDestroySampler(device, std::exchange(sampler, VK_NULL_HANDLE), nullptr);
}

void BlackHolePass::RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) {
    Utils::DebugUtils::LabelGuard labelGuard(commandBuffer, "BlackHolePass", 0.5F, 0.0F, 0.0F);

    if (isFirstRecording) {
#ifdef BLACK_HOLE_RAY_QUERY
        BuildBottomLevelASes(device, commandBuffer);
        BuildTopLevelAS(device, commandBuffer);
#endif // BLACK_HOLE_RAY_QUERY
        LoadCubeMap(device, commandBuffer);
        isFirstRecording = false;
    }

    // Force to undefined image layout, because of performance
    pFinalImage->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    Utils::ImagePipelineBarrier(commandBuffer, *pFinalImage, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);

    // Update Push Constants
    camera.Update();
    auto const &position = camera.GetPosition();
    auto const &direction = camera.GetDirection();

#pragma pack(push, 1)
    struct PushConst final {
        glm::vec3 cameraPos;
        float placeholder1 = 0.0F;
        glm::vec3 cameraDir;
#ifdef BLACK_HOLE_RAY_QUERY
        float placeholder2 = 0.0F;
        VkDeviceAddress texCoordsDeviceAddress[NUM_OF_BLAS_TEXTURES];
        VkDeviceAddress texCoordIndicesDeviceAddress[NUM_OF_BLAS_TEXTURES];
#endif // BLACK_HOLE_RAY_QUERY
    } pushConst {
        .cameraPos = position,
        .cameraDir = direction,
    };
#pragma pack(pop)

#ifdef BLACK_HOLE_RAY_QUERY
    std::memcpy(pushConst.texCoordsDeviceAddress, texCoordsDeviceAddress.data(),
        texCoordsDeviceAddress.size()*sizeof(VkDeviceAddress));
    std::memcpy(pushConst.texCoordIndicesDeviceAddress, texCoordIndicesDeviceAddress.data(),
        texCoordIndicesDeviceAddress.size()*sizeof(VkDeviceAddress));
#endif // BLACK_HOLE_RAY_QUERY

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
        0U, sizeof(PushConst), &pushConst);

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
    VkDescriptorPoolSize descriptorPoolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
#if defined(BLACK_HOLE_PRECOMPUTED)
            .descriptorCount = 3U
#elif defined(BLACK_HOLE_RAY_QUERY)
            .descriptorCount = 1U + NUM_OF_BLAS_TEXTURES
#else
            .descriptorCount = 1U
#endif // BLACK_HOLE_PRECOMPUTED
        },
#ifdef BLACK_HOLE_RAY_QUERY
        {
            .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1U
        },
#endif // BLACK_HOLE_RAY_QUERY
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1U
        }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCI {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .maxSets = 1U,
        .poolSizeCount = std::size(descriptorPoolSizes),
        .pPoolSizes = descriptorPoolSizes
    };

    VK_CALL(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, descriptorPool, "BlackHolePass::DescriptorPool");

#ifdef BLACK_HOLE_RAY_QUERY
    std::vector<VkSampler> linearSamplers(NUM_OF_BLAS_TEXTURES, sampler);
#endif // BLACK_HOLE_RAY_QUERY

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
#ifdef BLACK_HOLE_RAY_QUERY
        {
            .binding = BINDING_RAY_QUERY_TEXTURES,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = NUM_OF_BLAS_TEXTURES,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = linearSamplers.data()
        },
        {
            .binding = BINDING_RAY_QUERY_TLAS,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1U,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
#endif // BLACK_HOLE_RAY_QUERY
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
#ifdef BLACK_HOLE_PRECOMPUTED
        ,{
            .binding = BINDING_PRECOMPUTED_PHI_TEXTURE,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1U,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = &sampler
        },
        {
            .binding = BINDING_PRECOMPUTED_ACCR_DISK_DATA_TEXTURE,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1U,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = &sampler
        }
#endif // BLACK_HOLE_PRECOMPUTED
    };

#ifdef BLACK_HOLE_RAY_QUERY
    VkDescriptorBindingFlags bindingFlags[std::size(descriptorSetLayoutBindings)] = {};
    bindingFlags[0] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCI {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .pNext = nullptr,
        .bindingCount = std::size(bindingFlags),
        .pBindingFlags = bindingFlags
    };
#endif // BLACK_HOLE_RAY_QUERY

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
#ifdef BLACK_HOLE_RAY_QUERY
        .pNext = &bindingFlagsCI,
#else
        .pNext = nullptr,
#endif // BLACK_HOLE_RAY_QUERY
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
#ifdef BLACK_HOLE_PRECOMPUTED
        // Precomputed Phi Texture
        ,{
            .sampler = VK_NULL_HANDLE,
            .imageView = pPrecomputedPhiTexture->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        // Precomputed Data of Accretion Disk
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = pPrecomputedAccrDiskDataTexture->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        }
#endif // BLACK_HOLE_PRECOMPUTED
    };

#ifdef BLACK_HOLE_RAY_QUERY
    VkWriteDescriptorSetAccelerationStructureKHR tlasWriteDescriptor {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .pNext = nullptr,
        .accelerationStructureCount = 1U,
        .pAccelerationStructures = &tlasInfo.tlas
    };

    std::vector<VkDescriptorImageInfo> blasTexturesDescriptorImageInfos{};
    for (uint32_t i = 0U; i < blasInfos.size(); i++) {
        blasTexturesDescriptorImageInfos.push_back(VkDescriptorImageInfo{
            .sampler = VK_NULL_HANDLE,
            .imageView = blasInfos[i].pTexture->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        });
    }
#endif // BLACK_HOLE_RAY_QUERY

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
#ifdef BLACK_HOLE_PRECOMPUTED
        // Precomputed Phi Texture
        ,{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSet,
            .dstBinding = BINDING_PRECOMPUTED_PHI_TEXTURE,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &descriptorImageInfo[2],
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        },
        // Precomputed Data of Accretion Disk
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSet,
            .dstBinding = BINDING_PRECOMPUTED_ACCR_DISK_DATA_TEXTURE,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &descriptorImageInfo[3],
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        }
#endif // BLACK_HOLE_PRECOMPUTED
#ifdef BLACK_HOLE_RAY_QUERY
        // Top Level Acceleration Structure
        ,{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = &tlasWriteDescriptor,
            .dstSet = descriptorSet,
            .dstBinding = BINDING_RAY_QUERY_TLAS,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .pImageInfo = nullptr,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        },
        // Bottom Level Acceleration Structure Texture
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = &tlasWriteDescriptor,
            .dstSet = descriptorSet,
            .dstBinding = BINDING_RAY_QUERY_TEXTURES,
            .dstArrayElement = 0U,
            .descriptorCount = static_cast<uint32_t>(blasTexturesDescriptorImageInfos.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = blasTexturesDescriptorImageInfos.data(),
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        }
#endif // BLACK_HOLE_RAY_QUERY
    };

    vkUpdateDescriptorSets(device, std::size(writeDescriptors), writeDescriptors, 0U, nullptr);
}

void BlackHolePass::InitPipeline(VkDevice device) {
    VkPushConstantRange pushConstantRanges[] = {
        {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0U,
#ifdef BLACK_HOLE_RAY_QUERY
            .size = 128U
#else
            .size = 28U
#endif // BLACK_HOLE_RAY_QUERY
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

#ifdef BLACK_HOLE_PRECOMPUTED
    Utils::ShaderModule blackHoleComp = Utils::ShaderModule(device, Utils::SHADER_LIST_ID::BLACK_HOLE_PRECOMPUTED_COMP);
#elif defined(BLACK_HOLE_RAY_QUERY)
    Utils::ShaderModule blackHoleComp = Utils::ShaderModule(device, Utils::SHADER_LIST_ID::BLACK_HOLE_RAY_QUERY_COMP);
#elif defined(BLACK_HOLE_RAY_MARCHING_RK1)
    Utils::ShaderModule blackHoleComp = Utils::ShaderModule(device, Utils::SHADER_LIST_ID::BLACK_HOLE_RAY_MARCHING_RK1_COMP);
#elif defined(BLACK_HOLE_RAY_MARCHING_RK2)
    Utils::ShaderModule blackHoleComp = Utils::ShaderModule(device, Utils::SHADER_LIST_ID::BLACK_HOLE_RAY_MARCHING_RK2_COMP);
#else // defined(BLACK_HOLE_RAY_MARCHING_RK4)
    Utils::ShaderModule blackHoleComp = Utils::ShaderModule(device, Utils::SHADER_LIST_ID::BLACK_HOLE_RAY_MARCHING_RK4_COMP);
#endif

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
    Utils::DebugUtils::LabelGuard loadGuard(commandBuffer, "BlackHolePass::LoadCubeMap", 0.0F, 1.0F, 0.0F);

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

#ifdef BLACK_HOLE_RAY_QUERY

void BlackHolePass::AllocateBottomLevelASes(VkDevice device, Utils::GPUAllocator &gpuAllocator, uint32_t num) {
    blasInfos.resize(num);

    for (uint32_t idx = 0U; idx < num; idx++) {
        auto &blasInfo = blasInfos[idx];
        auto &objData = blasInfo.objData;
        objData.Init(std::format("objects/obj{}.obj", idx));
        blasInfo.transformMatrix = blasTransformMatrices[idx];

        VkAccelerationStructureGeometryTrianglesDataKHR const geometryTrianglesData {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
            .pNext = nullptr,
            .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
            .vertexData = {
                .deviceAddress = 0ULL // Will set later
            },
            .vertexStride = 3U*sizeof(float),
            .maxVertex = static_cast<uint32_t>(objData.GetVertices().size()) - 1U,
            .indexType = VK_INDEX_TYPE_UINT32,
            .indexData = {
                .deviceAddress = 0ULL // Will set later
            },
            .transformData = {
                .deviceAddress = 0ULL
            }
        };

        VkAccelerationStructureGeometryKHR &geometry = blasInfo.geometry;
        geometry = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .pNext = nullptr,
            .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            .geometry = {
                .triangles = geometryTrianglesData
            },
            .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
        };

        VkAccelerationStructureBuildRangeInfoKHR &buildRangeInfo = blasInfo.buildRangeInfo;
        buildRangeInfo = {
            .primitiveCount = objData.GetNumOfTriangles(),
            .primitiveOffset = 0U,
            .firstVertex = 0U,
            .transformOffset = 0U
        };

        VkAccelerationStructureBuildGeometryInfoKHR &buildGeometryInfo = blasInfo.buildGeometryInfo;
        buildGeometryInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .pNext = nullptr,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .srcAccelerationStructure = VK_NULL_HANDLE,
            .dstAccelerationStructure = VK_NULL_HANDLE,
            .geometryCount = 1U,
            .pGeometries = &geometry,
            .ppGeometries = nullptr,
            .scratchData = {
                .deviceAddress = 0ULL // Will set later
            }
        };

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
            .pNext = nullptr
        };

        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildGeometryInfo, &buildRangeInfo.primitiveCount, &sizeInfo);

        Utils::CreateBufferInfo bottomLevelASBufferCI {
            .size = sizeInfo.accelerationStructureSize,
            .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            .useDeviceAddressableMemory = true,
            .name = std::format("BlackHolePass::Bottom Level AS Buffer [{}]", idx)
        };

        Buffer* &pUnderlyingBuffer = blasInfo.pUnderlyingBLASBuffer;
        pUnderlyingBuffer = &gpuAllocator.AddBuffer(device, bottomLevelASBufferCI);

        Utils::CreateBufferInfo vertexBufferCI {
            .size = static_cast<VkDeviceSize>(objData.GetVertices().size()*sizeof(float)),
            .usage = (VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR),
            .useDeviceAddressableMemory = true,
            .name = std::format("BlackHolePass::Bottom Level AS Vertex Buffer [{}]", idx)
        };
        blasInfo.pVertexBuffer = &gpuAllocator.AddBuffer(device, vertexBufferCI);

        Utils::CreateBufferInfo indexBufferCI {
            .size = static_cast<VkDeviceSize>(objData.GetVertexIndices().size()*sizeof(uint32_t)),
            .usage = (VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR),
            .useDeviceAddressableMemory = true,
            .name = std::format("BlackHolePass::Bottom Level AS Index Buffer [{}]", idx)
        };
        blasInfo.pIndexBuffer = &gpuAllocator.AddBuffer(device, indexBufferCI);

        VkAccelerationStructureCreateInfoKHR const accelerationStructureCI {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .createFlags = 0U,
            .buffer = pUnderlyingBuffer->buffer,
            .offset = 0U,
            .size = sizeInfo.accelerationStructureSize,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .deviceAddress = 0ULL
        };

        VkAccelerationStructureKHR &blas = blasInfo.blas;
        VK_CALL(vkCreateAccelerationStructureKHR(device, &accelerationStructureCI, nullptr, &blas));
        Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, blas,
            std::format("BlackHolePass::Bottom Level AS [{}]", idx).c_str());

        scratchBufferSize = std::max(scratchBufferSize, sizeInfo.buildScratchSize);

        buildGeometryInfo.dstAccelerationStructure = blas;

        // Texture Coordinates Zone
        Utils::CreateBufferInfo texCoordsBufferCI {
            .size = static_cast<VkDeviceSize>(objData.GetTexCoords().size()*sizeof(float)),
            .usage = (VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
            .useDeviceAddressableMemory = true,
            .name = std::format("BlackHolePass::Bottom Level AS Texture Coordinates Buffer [{}]", idx)
        };
        blasInfo.pTexCoordsBuffer = &gpuAllocator.AddBuffer(device, texCoordsBufferCI);

        Utils::CreateBufferInfo texCoordIndicesBufferCI {
            .size = static_cast<VkDeviceSize>(objData.GetTexCoordIndices().size()*sizeof(uint32_t)),
            .usage = (VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
            .useDeviceAddressableMemory = true,
            .name = std::format("BlackHolePass::Bottom Level AS Texture Coordinate Index Buffer [{}]", idx)
        };
        blasInfo.pTexCoordIndicesBuffer = &gpuAllocator.AddBuffer(device, texCoordIndicesBufferCI);

        // Texture Zone
        blasInfo.textureFileName = std::format("textures/obj{}.png", idx);
        int isize_x, isize_y;
        stbi_info(blasInfo.textureFileName.c_str(), &isize_x, &isize_y, nullptr);
        uint32_t size_x = isize_x, size_y = isize_y;

        Utils::CreateBufferInfo stagingBufferCI {
            .size = size_x*size_y*4U*sizeof(uint8_t),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .name = std::format("BlackHolePass::Bottom Level AS Texture Staging Buffer [{}]", idx)
        };
        blasInfo.pStagingBuffer = &gpuAllocator.AddBuffer(device, stagingBufferCI, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 0U);

        Utils::CreateImageInfo textureCI {
            .extent = {
                .width = size_x,
                .height = size_y,
                .depth = 1U
            },
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .name = std::format("BlackHolePass::Bottom Level AS Texture [{}]", idx)
        };
        blasInfo.pTexture = &gpuAllocator.AddImage(device, textureCI);
    }
}

void BlackHolePass::BuildBottomLevelASes(VkDevice device, VkCommandBuffer commandBuffer) {
    Utils::DebugUtils::LabelGuard loadGuard(commandBuffer, "BuildBottomLevelAS", 0.5F, 0.0F, 0.5F);

    VkBufferDeviceAddressInfo bufferDeviceAddressInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = pScratchBuffer->buffer
    };
    VkDeviceAddress scratchBufferDeviceAddress = ((vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo) + 255ULL) & (~255ULL));

    for (auto &blasInfo : blasInfos) {
        VkBuffer vertexBuffer = blasInfo.pVertexBuffer->buffer;
        auto const &vertexData = blasInfo.objData.GetVertices();
        vkCmdUpdateBuffer(commandBuffer, vertexBuffer, 0ULL, vertexData.size()*sizeof(float), vertexData.data());

        VkBuffer indexBuffer = blasInfo.pIndexBuffer->buffer;
        auto const &indexData = blasInfo.objData.GetVertexIndices();
        vkCmdUpdateBuffer(commandBuffer, indexBuffer, 0ULL, indexData.size()*sizeof(uint32_t), indexData.data());

        Utils::MemoryPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT);

        VkBuffer texCoordsBuffer = blasInfo.pTexCoordsBuffer->buffer;
        auto const &texCoordsData = blasInfo.objData.GetTexCoords();
        vkCmdUpdateBuffer(commandBuffer, texCoordsBuffer, 0ULL, texCoordsData.size()*sizeof(float), texCoordsData.data());

        VkBuffer texCoordIndicesBuffer = blasInfo.pTexCoordIndicesBuffer->buffer;
        auto const &texCoordIndicesData = blasInfo.objData.GetTexCoordIndices();
        vkCmdUpdateBuffer(commandBuffer, texCoordIndicesBuffer, 0ULL, texCoordIndicesData.size()*sizeof(uint32_t), texCoordIndicesData.data());

        Utils::MemoryPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

        bufferDeviceAddressInfo.buffer = texCoordsBuffer;
        texCoordsDeviceAddress.push_back(vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo));
        bufferDeviceAddressInfo.buffer = texCoordIndicesBuffer;
        texCoordIndicesDeviceAddress.push_back(vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo));

        VkAccelerationStructureGeometryTrianglesDataKHR &geometryTrianglesData = blasInfo.geometry.geometry.triangles;
        bufferDeviceAddressInfo.buffer = vertexBuffer;
        geometryTrianglesData.vertexData.deviceAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);
        bufferDeviceAddressInfo.buffer = indexBuffer;
        geometryTrianglesData.indexData.deviceAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);
        blasInfo.buildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR const *pBuildRangeInfo = &blasInfo.buildRangeInfo;
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1U, &blasInfo.buildGeometryInfo, &pBuildRangeInfo);

        Utils::MemoryPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);

        // Texture Zone
        Image* &pTexture = blasInfo.pTexture;
        Buffer* &pStagingBuffer = blasInfo.pStagingBuffer;
        const uint32_t size_x = pTexture->size.width, size_y = pTexture->size.height;
        int x = size_x, y = size_y, channels = 4U;
        uint8_t *copyData = stbi_load(blasInfo.textureFileName.c_str(), &x, &y, &channels, 0U);
        VkDeviceSize copyDataSize = size_x*size_y*4U*sizeof(uint8_t);

        Utils::CopyMemoryIntoStagingBuffer(device, *pStagingBuffer, copyData, copyDataSize);

        Utils::ImagePipelineBarrier(commandBuffer, *pTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);

        stbi_image_free(copyData);

        // Copy buffer data to image data
        VkBufferImageCopy bufferImageCopy {
            .bufferOffset = 0U,
            .bufferRowLength = 0U,
            .bufferImageHeight = 0U,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0U,
                .baseArrayLayer = 0U,
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

        vkCmdCopyBufferToImage(commandBuffer, pStagingBuffer->buffer, pTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &bufferImageCopy);

        Utils::ImagePipelineBarrier(commandBuffer, *pTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
    }
}

void BlackHolePass::AllocateTopLevelAS(VkDevice device, Utils::GPUAllocator &gpuAllocator) {
    if (blasInfos.size() > NUM_OF_BLAS_TEXTURES) {
        // TODO: Fix this
        throw std::runtime_error("BlackHolePass::AllocateTopLevelAS: Cannot use more than 6 BLASes");
    }

    for (uint32_t i = 0U; i < blasInfos.size(); i++) {
        auto &blasInfo = blasInfos[i];

        tlasInfo.instances.push_back(VkAccelerationStructureInstanceKHR{
            .transform = blasInfo.transformMatrix,
            .instanceCustomIndex = i,
            .mask = 0xFFU,
            .instanceShaderBindingTableRecordOffset = 0U,
            .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference = 0ULL // Will set later
        });
    }

    VkAccelerationStructureGeometryInstancesDataKHR const geometryInstancesData {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .pNext = nullptr,
        .arrayOfPointers = VK_FALSE,
        .data = {
            .deviceAddress = 0ULL // Will set later
        }
    };

    VkAccelerationStructureGeometryKHR &geometry = tlasInfo.geometry;
    geometry = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .pNext = nullptr,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = {
            .instances = geometryInstancesData
        },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    VkAccelerationStructureBuildRangeInfoKHR &buildRangeInfo = tlasInfo.buildRangeInfo;
    buildRangeInfo = {
        .primitiveCount = static_cast<uint32_t>(tlasInfo.instances.size()),
        .primitiveOffset = 0U,
        .firstVertex = 0U,
        .transformOffset = 0U
    };

    VkAccelerationStructureBuildGeometryInfoKHR &buildGeometryInfo = tlasInfo.buildGeometryInfo;
    buildGeometryInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .pNext = nullptr,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .srcAccelerationStructure = VK_NULL_HANDLE,
        .dstAccelerationStructure = VK_NULL_HANDLE,
        .geometryCount = 1U,
        .pGeometries = &geometry,
        .ppGeometries = nullptr,
        .scratchData = {
            .deviceAddress = 0ULL // Will set later
        }
    };

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        .pNext = nullptr
    };

    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildGeometryInfo, &buildRangeInfo.primitiveCount, &sizeInfo);

    Utils::CreateBufferInfo topLevelASBufferCI {
        .size = sizeInfo.accelerationStructureSize,
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        .name = "BlackHolePass::Top Level AS Buffer"
    };

    Buffer* &pUnderlyingBuffer = tlasInfo.pUnderlyingBLASBuffer;
    pUnderlyingBuffer = &gpuAllocator.AddBuffer(device, topLevelASBufferCI);

    VkAccelerationStructureCreateInfoKHR const accelerationStructureCI {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .createFlags = 0U,
        .buffer = pUnderlyingBuffer->buffer,
        .offset = 0U,
        .size = sizeInfo.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .deviceAddress = 0ULL
    };

    VkAccelerationStructureKHR &tlas = tlasInfo.tlas;
    VK_CALL(vkCreateAccelerationStructureKHR(device, &accelerationStructureCI, nullptr, &tlas));
    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, tlas,
        "BlackHolePass::Top Level AS");

    scratchBufferSize = std::max(scratchBufferSize, sizeInfo.buildScratchSize);
    Utils::CreateBufferInfo scratchBufferCI {
        .size = ((scratchBufferSize + 255ULL) & (~255ULL)),
        .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .useDeviceAddressableMemory = true,
        .name = "BlackHolePass::ScratchBuffer"
    };
    pScratchBuffer = &gpuAllocator.AddBuffer(device, scratchBufferCI);

    Utils::CreateBufferInfo instanceBufferCI {
        .size = sizeof(VkAccelerationStructureInstanceKHR)*tlasInfo.instances.size(),
        .usage = (VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR),
        .useDeviceAddressableMemory = true,
        .name = "BlackHolePass::Top Level AS Instance Buffer"
    };
    tlasInfo.pInstanceBuffer = &gpuAllocator.AddBuffer(device, instanceBufferCI);

    buildGeometryInfo.dstAccelerationStructure = tlas;
}

void BlackHolePass::BuildTopLevelAS(VkDevice device, VkCommandBuffer commandBuffer) {
    Utils::DebugUtils::LabelGuard loadGuard(commandBuffer, "BuildTopLevelAS", 1.0F, 0.0F, 1.0F);

    for (uint32_t i = 0U; i < tlasInfo.instances.size(); i++) {
        VkAccelerationStructureDeviceAddressInfoKHR const deviceAddressInfo {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .pNext = nullptr,
            .accelerationStructure = blasInfos[i].blas
        };

        VkDeviceAddress blasDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &deviceAddressInfo);

        tlasInfo.instances[i].accelerationStructureReference = blasDeviceAddress;
    }

    VkBuffer instanceBuffer = tlasInfo.pInstanceBuffer->buffer;
    vkCmdUpdateBuffer(commandBuffer, instanceBuffer, 0ULL,
        sizeof(VkAccelerationStructureInstanceKHR)*tlasInfo.instances.size(), tlasInfo.instances.data());

    Utils::MemoryPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_ACCESS_SHADER_READ_BIT);

    VkBufferDeviceAddressInfo bufferDeviceAddressInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = pScratchBuffer->buffer
    };
    VkDeviceAddress scratchBufferDeviceAddress = ((vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo) + 255ULL) & (~255ULL));
    tlasInfo.buildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

    bufferDeviceAddressInfo.buffer = tlasInfo.pInstanceBuffer->buffer;
    tlasInfo.geometry.geometry.instances.data.deviceAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);

    VkAccelerationStructureBuildRangeInfoKHR const *pBuildRangeInfo = &tlasInfo.buildRangeInfo;
    vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1U, &tlasInfo.buildGeometryInfo, &pBuildRangeInfo);

    Utils::MemoryPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
}

#endif // BLACK_HOLE_RAY_QUERY

}

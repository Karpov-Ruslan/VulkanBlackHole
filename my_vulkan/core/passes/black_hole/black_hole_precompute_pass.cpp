#include "black_hole_precompute_pass.hpp"

#include "my_vulkan/shaders/shaders_list.hpp"
#include "my_vulkan/shaders/black_hole.in"

#include "my_vulkan/vulkan_functions.hpp"

#include "common.hpp"

#include <utility>

namespace KRV {

void BlackHolePrecomputePass::AllocateResources(VkDevice device, Utils::GPUAllocator& gpuAllocator) {
    Utils::CreateImageInfo precomputedPhiTextureCI {
        .format = VK_FORMAT_R32G32_SFLOAT,
        .extent {
            .width = PRECOMPUTED_PHI_TEXTURE_WIDTH,
            .height = PRECOMPUTED_PHI_TEXTURE_HEIGHT,
            .depth = 1U
        },
        .usage = (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
        .name = PRECOMPUTED_PHI_TEXTURE_NAME
    };

    pPrecomputedPhiTexture = &gpuAllocator.AddImage(device, precomputedPhiTextureCI);

    Utils::CreateImageInfo precomputedAccrDiskDataTextureCI {
        .type = VK_IMAGE_TYPE_3D,
        .viewType = VK_IMAGE_VIEW_TYPE_3D,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .extent {
            .width = PRECOMPUTED_ACCR_DISK_DATA_TEXTURE_WIDTH,
            .height = PRECOMPUTED_ACCR_DISK_DATA_TEXTURE_HEIGHT,
            .depth = PRECOMPUTED_ACCR_DISK_DATA_TEXTURE_DEPTH
        },
        .usage = (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
        .name = PRECOMPUTED_ACCR_DISK_DATA_TEXTURE_NAME
    };

    pPrecomputedAccrDiskDataTexture = &gpuAllocator.AddImage(device, precomputedAccrDiskDataTextureCI);
}

void BlackHolePrecomputePass::Init(VkDevice device) {
    InitDescriptorSet(device);
    InitPipeline(device);
}

void BlackHolePrecomputePass::Destroy(VkDevice device) {
    vkDestroyPipeline(device, std::exchange(precomputePhiPipeline, VK_NULL_HANDLE), nullptr);
    vkDestroyPipeline(device, std::exchange(precomputeAccrDiskDataPipeline, VK_NULL_HANDLE), nullptr);
    vkDestroyPipelineLayout(device, std::exchange(pipelineLayout, VK_NULL_HANDLE), nullptr);
    vkDestroyDescriptorSetLayout(device, std::exchange(descriptorSetLayout, VK_NULL_HANDLE), nullptr);
    vkDestroyDescriptorPool(device, std::exchange(descriptorPool, VK_NULL_HANDLE), nullptr);
}

void BlackHolePrecomputePass::RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) {
    if (!isFirstRecording) {
        return;
    }

    Utils::DebugUtils::LabelGuard labelGuard(commandBuffer, "BlackHolePrecomputePass", 0.0F, 0.5F, 0.0F);

    Utils::ImagePipelineBarrier(commandBuffer, *pPrecomputedPhiTexture,
        VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);
    Utils::ImagePipelineBarrier(commandBuffer, *pPrecomputedAccrDiskDataTexture,
        VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, precomputePhiPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0U, 1U, &descriptorSet, 0U, nullptr);
    vkCmdDispatch(commandBuffer, PRECOMPUTED_PHI_TEXTURE_WIDTH/LOCAL_SIZE_X, PRECOMPUTED_PHI_TEXTURE_HEIGHT/LOCAL_SIZE_Y, 1U);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, precomputeAccrDiskDataPipeline);
    // There is no need to use `vkCmdBindDescriptorSets` because pipelineLayout is the same
    vkCmdDispatch(commandBuffer, PRECOMPUTED_ACCR_DISK_DATA_TEXTURE_WIDTH/LOCAL_SIZE_X,
        PRECOMPUTED_ACCR_DISK_DATA_TEXTURE_HEIGHT/LOCAL_SIZE_Y, PRECOMPUTED_ACCR_DISK_DATA_TEXTURE_DEPTH);

    Utils::ImagePipelineBarrier(commandBuffer, *pPrecomputedPhiTexture,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
    Utils::ImagePipelineBarrier(commandBuffer, *pPrecomputedAccrDiskDataTexture,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

    isFirstRecording = false;
}

void BlackHolePrecomputePass::InitDescriptorSet(VkDevice device) {
    VkDescriptorPoolSize descriptorPoolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 2U
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

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, descriptorPool, "BlackHolePrecomputePass::DescriptorPool");

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
        {
            .binding = BINDING_PRECOMPUTED_PHI_TEXTURE,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1U,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        {
            .binding = BINDING_PRECOMPUTED_ACCR_DISK_DATA_TEXTURE,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1U,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
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

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, descriptorSetLayout, "BlackHolePrecomputePass::DescriptorSetLayout");

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1U,
        .pSetLayouts = &descriptorSetLayout
    };

    VK_CALL(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet))

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, descriptorSet, "BlackHolePrecomputePass::DescriptorSet");

    VkDescriptorImageInfo descriptorImageInfos[] = {
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = pPrecomputedPhiTexture->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        },
        {
            .sampler = VK_NULL_HANDLE,
            .imageView = pPrecomputedAccrDiskDataTexture->imageView,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        },
    };

    VkWriteDescriptorSet writeDescriptors[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSet,
            .dstBinding = BINDING_PRECOMPUTED_PHI_TEXTURE,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &descriptorImageInfos[0],
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptorSet,
            .dstBinding = BINDING_PRECOMPUTED_ACCR_DISK_DATA_TEXTURE,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &descriptorImageInfos[1],
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        }
    };

    vkUpdateDescriptorSets(device, std::size(writeDescriptors), writeDescriptors, 0U, nullptr);
}

void BlackHolePrecomputePass::InitPipeline(VkDevice device) {
    VkPipelineLayoutCreateInfo pipelineLayoutCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .setLayoutCount = 1U,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 0U,
        .pPushConstantRanges = nullptr
    };

    VK_CALL(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, pipelineLayout, "BlackHolePrecomputePass::PipelineLayout");

    Utils::ShaderModule blackHolePrecomputePhiComp = Utils::ShaderModule(device,
        Utils::SHADER_LIST_ID::BLACK_HOLE_PRECOMPUTE_PHI_TEXTURE_COMP);

    VkComputePipelineCreateInfo pipelineCI {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0U,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = blackHolePrecomputePhiComp,
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
        .layout = pipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0U
    };

    VK_CALL(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1U, &pipelineCI, nullptr, &precomputePhiPipeline));

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_PIPELINE, precomputePhiPipeline,
        "BlackHolePrecomputePass::PrecomputePhiPipeline");

    Utils::ShaderModule blackHolePrecomputeAccrDiskDataComp = Utils::ShaderModule(device,
        Utils::SHADER_LIST_ID::BLACK_HOLE_PRECOMPUTE_ACCR_DISK_DATA_TEXTURE_COMP);

    pipelineCI.stage.module = blackHolePrecomputeAccrDiskDataComp;

    VK_CALL(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1U, &pipelineCI, nullptr, &precomputeAccrDiskDataPipeline));

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_PIPELINE, precomputePhiPipeline,
        "BlackHolePrecomputePass::PrecomputeAccrDiskDataPipeline");
}


}
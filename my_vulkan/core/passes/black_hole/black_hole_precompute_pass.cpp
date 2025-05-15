#include "black_hole_precompute_pass.hpp"

#include "my_vulkan/shaders/shaders_list.hpp"
#include "my_vulkan/shaders/black_hole.in"

#include "my_vulkan/vulkan_functions.hpp"

#include "common.hpp"

namespace KRV {

void BlackHolePrecomputePass::AllocateResources(VkDevice device, Utils::GPUAllocator& gpuAllocator) {
    Utils::CreateImageInfo precomputedTextureCI {
        .format = VK_FORMAT_R32_SFLOAT,
        .extent {
            .width = PRECOMPUTED_TEXTURE_WIDTH,
            .height = PRECOMPUTED_TEXTURE_HEIGHT,
            .depth = 1U
        },
        .usage = (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
        .name = PRECOMPUTED_TEXTURE_NAME
    };

    pPrecomputedTexture = &gpuAllocator.AddImage(device, precomputedTextureCI);
}

void BlackHolePrecomputePass::Init(VkDevice device) {
    InitDescriptorSet(device);
    InitPipeline(device);
}

void BlackHolePrecomputePass::Destroy(VkDevice device) {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void BlackHolePrecomputePass::RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) {
    if (!isFirstRecording) {
        return;
    }

    Utils::DebugUtils::LabelGuard labelGuard(commandBuffer, "BlackHolePrecomputePass", 0.0F, 0.5F, 0.0F);

    Utils::ImagePipelineBarrier(commandBuffer, *pPrecomputedTexture,
        VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0U, 1U, &descriptorSet, 0U, nullptr);
    vkCmdDispatch(commandBuffer, PRECOMPUTED_TEXTURE_WIDTH/LOCAL_SIZE_X, PRECOMPUTED_TEXTURE_HEIGHT/LOCAL_SIZE_Y, 1U);

    Utils::ImagePipelineBarrier(commandBuffer, *pPrecomputedTexture,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

    isFirstRecording = false;
}

void BlackHolePrecomputePass::InitDescriptorSet(VkDevice device) {
    VkDescriptorPoolSize descriptorPoolSizes[] = {
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

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, descriptorPool, "BlackHolePrecomputePass::DescriptorPool");

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
        {
            .binding = BINDING_PRECOMPUTED_TEXTURE,
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

    VkDescriptorImageInfo descriptorImageInfo = {
        .sampler = VK_NULL_HANDLE,
        .imageView = pPrecomputedTexture->imageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet writeDescriptor = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptorSet,
        .dstBinding = BINDING_PRECOMPUTED_TEXTURE,
        .dstArrayElement = 0U,
        .descriptorCount = 1U,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &descriptorImageInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(device, 1U, &writeDescriptor, 0U, nullptr);
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


    Utils::ShaderModule blackHolePrecomputeComp = Utils::ShaderModule(device,
        Utils::SHADER_LIST_ID::BLACK_HOLE_PRECOMPUTE_PRECOMPUTED_TEXTURE_COMP);

    VkPipelineShaderStageCreateInfo stageCI {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = blackHolePrecomputeComp,
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

    Utils::DebugUtils::Name(device, VK_OBJECT_TYPE_PIPELINE, pipeline, "BlackHolePrecomputePass::Pipeline");
}


}
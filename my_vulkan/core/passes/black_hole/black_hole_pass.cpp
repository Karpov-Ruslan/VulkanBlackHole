#include "black_hole_pass.hpp"

#include "my_vulkan/shaders/shaders_list.hpp"
#include "my_vulkan/shaders/black_hole.in"

#include "my_vulkan/vulkan_functions.hpp"

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
}

void BlackHolePass::Init(VkDevice device) {
    InitDescriptorSet(device);
    InitPipeline(device);
}

void BlackHolePass::Destroy(VkDevice device) {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void BlackHolePass::RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) {
    Utils::DebugUtils::LabelGuard labelGuard(commandBuffer, "BlackHolePass", 0.5F, 0.0F, 0.0F);

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

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding {
        .binding = 0U,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1U,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .bindingCount = 1U,
        .pBindings = &descriptorSetLayoutBinding
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

    VkDescriptorImageInfo descriptorImageInfo {
        .sampler = VK_NULL_HANDLE,
        .imageView = pFinalImage->imageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet writeDescriptors {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptorSet,
        .dstBinding = 0U,
        .dstArrayElement = 0U,
        .descriptorCount = 1U,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &descriptorImageInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(device, 1U, &writeDescriptors, 0U, nullptr);
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

}

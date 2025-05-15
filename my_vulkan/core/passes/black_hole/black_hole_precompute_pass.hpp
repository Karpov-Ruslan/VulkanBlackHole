#pragma once

#include "../base_pass.hpp"

namespace KRV {

class BlackHolePrecomputePass final : public BasePass {
public:
    BlackHolePrecomputePass() = default;

    BlackHolePrecomputePass(BlackHolePrecomputePass const &) = delete;
    BlackHolePrecomputePass& operator=(BlackHolePrecomputePass const &) = delete;
    BlackHolePrecomputePass(BlackHolePrecomputePass &&) = delete;
    BlackHolePrecomputePass& operator=(BlackHolePrecomputePass &&) = delete;

    ~BlackHolePrecomputePass() = default;

    void AllocateResources(VkDevice device, Utils::GPUAllocator& gpuAllocator) override;
    void Init(VkDevice device) override;
    void Destroy(VkDevice device) override;
    void RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) override;

private:
    void InitDescriptorSet(VkDevice device);
    void InitPipeline(VkDevice device);

    Image *pPrecomputedTexture = nullptr;

    bool isFirstRecording = true;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

}
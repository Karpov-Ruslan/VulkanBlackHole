#pragma once

#include "../base_pass.hpp"
#include "utils/camera.hpp"

namespace KRV {

class BlackHolePass final : public BasePass {
public:
    BlackHolePass() = default;

    BlackHolePass(BlackHolePass const &) = delete;
    BlackHolePass& operator=(BlackHolePass const &) = delete;
    BlackHolePass(BlackHolePass &&) = delete;
    BlackHolePass& operator=(BlackHolePass &&) = delete;

    ~BlackHolePass() = default;

    void AllocateResources(VkDevice device, Utils::GPUAllocator& gpuAllocator) override;
    void Init(VkDevice device) override;
    void Destroy(VkDevice device) override;
    void RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) override;

    Image& GetFinalImage();

private:
    void InitDescriptorSet(VkDevice device);
    void InitPipeline(VkDevice device);

    Image *pFinalImage = nullptr;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    Camera camera = Camera(glm::vec3(-1.0F, 0.0F, 0.0F), glm::vec3(1.0F, 0.0F, 0.0F), 0.1F, 1.0F, 1.57F);
};

}
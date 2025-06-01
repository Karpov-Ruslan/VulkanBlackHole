#pragma once

#include "../base_pass.hpp"
#include "utils/camera.hpp"
#include "utils/obj_data.hpp"
namespace KRV {

class BlackHolePass final : public BasePass {
public:
    BlackHolePass() = default;

    BlackHolePass(BlackHolePass const &) = delete;
    BlackHolePass& operator=(BlackHolePass const &) = delete;
    BlackHolePass(BlackHolePass &&) = delete;
    BlackHolePass& operator=(BlackHolePass &&) = delete;

    ~BlackHolePass() = default;

    void AllocateResources(VkDevice device, Utils::GPUAllocator &gpuAllocator) override;
    void Init(VkDevice device) override;
    void Destroy(VkDevice device) override;
    void RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) override;

    Image& GetFinalImage();

private:
    void InitSampler(VkDevice device);
    void InitDescriptorSet(VkDevice device);
    void InitPipeline(VkDevice device);

    void AllocateCubeMap(VkDevice device, Utils::GPUAllocator &gpuAllocator);
    void LoadCubeMap(VkDevice device, VkCommandBuffer commandBuffer);

#ifdef BLACK_HOLE_RAY_QUERY
    void AllocateBottomLevelAS(VkDevice device, Utils::GPUAllocator &gpuAllocator);
    void BuildBottomLevelASes(VkDevice device, VkCommandBuffer commandBuffer);

    void AllocateTopLevelAS(VkDevice device, Utils::GPUAllocator &gpuAllocator);
    void BuildTopLevelAS(VkDevice device, VkCommandBuffer commandBuffer);
#endif // BLACK_HOLE_RAY_QUERY

    Image *pFinalImage = nullptr;

    Image *pCubeMap = nullptr;
    Buffer *pStagingBuffer = nullptr;
    bool isFirstRecording = true;

#if defined(BLACK_HOLE_PRECOMPUTED)
    // Just take it from precompute pass, there is no allocation of this resource.
    Image *pPrecomputedPhiTexture = nullptr;
    Image *pPrecomputedAccrDiskDataTexture = nullptr;
#elif defined(BLACK_HOLE_RAY_QUERY)
    struct BlasInfo final {
        OBJData objData;
        VkTransformMatrixKHR transformMatrix{};
        VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        VkAccelerationStructureGeometryKHR geometry{};
        VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
        VkAccelerationStructureKHR blas = VK_NULL_HANDLE;
        Buffer *pVertexBuffer = nullptr;
        Buffer *pIndexBuffer = nullptr;
        Buffer *pUnderlyingBLASBuffer = nullptr;
    };

    struct TlasInfo final {
        std::vector<VkAccelerationStructureInstanceKHR> instances{};
        VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        VkAccelerationStructureGeometryKHR geometry{};
        VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
        VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
        Buffer *pInstanceBuffer = nullptr;
        Buffer *pUnderlyingBLASBuffer = nullptr;
    };

    std::vector<BlasInfo> blasInfos;
    TlasInfo tlasInfo{};
    // General scratch buffer for all acceleration structures.
    VkDeviceSize scratchBufferSize = 0ULL;
    Buffer *pScratchBuffer = nullptr;
#endif // BLACK_HOLE_PRECOMPUTED, BLACK_HOLE_RAY_QUERY

    VkSampler sampler = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    Camera camera = Camera(glm::vec3(-0.2F, 0.0F, 0.05F), glm::vec3(1.0F, 0.0F, 0.0F), 0.1F, 1.0F, 1.57F);
};

}
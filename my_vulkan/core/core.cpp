#include "core.hpp"

#include "passes/black_hole/black_hole_pass.hpp"

#ifdef BLACK_HOLE_PRECOMPUTED

#include "passes/black_hole/black_hole_precompute_pass.hpp"

#endif // BLACK_HOLE_PRECOMPUTED

namespace KRV {

Core::Core() {

#ifdef BLACK_HOLE_PRECOMPUTED

    passes.emplace_back(std::make_unique<BlackHolePrecomputePass>());

#endif // BLACK_HOLE_PRECOMPUTED

    // Black Hole Pass
    passes.emplace_back(std::make_unique<BlackHolePass>());
}

void Core::Init(VkPhysicalDevice physicalDevice, VkDevice device) {
    // Firstly, allocate Vulkan resources
    gpuAllocator.Init(physicalDevice);
    for (auto &pPass : passes) {
        pPass->AllocateResources(device, gpuAllocator);
    }
    gpuAllocator.PresentResources(device);

    // Secondly, just init passes
    for (auto &pPass : passes) {
        pPass->Init(device);
    }
}

void Core::Destroy(VkDevice device) {
    for (auto &pPass : passes) {
        pPass->Destroy(device);
    }

    gpuAllocator.Destroy(device);
}

Image& Core::RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) {
    for (auto &pPass : passes) {
        pPass->RecordCommandBuffer(device, commandBuffer);
    }

    return static_cast<BlackHolePass*>(passes.back().get())->GetFinalImage();
}

}

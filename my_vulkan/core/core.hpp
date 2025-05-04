#pragma once

#include <memory>
#include "my_vulkan/gpu_allocator.hpp"
#include "passes/base_pass.hpp"

namespace KRV {

class Core final {
public:
    Core();

    Core(Core const &) = delete;
    Core& operator=(Core const &) = delete;
    Core(Core &&) = delete;
    Core& operator=(Core &&) = delete;

    ~Core() = default;

    void Init(VkPhysicalDevice physicalDevice, VkDevice device);

    void Destroy(VkDevice device);

    Image& RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer);

private:
    Utils::GPUAllocator gpuAllocator{};

    // Passes
    std::vector<std::unique_ptr<BasePass>> passes{};
};

}

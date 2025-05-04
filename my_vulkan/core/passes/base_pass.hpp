#pragma once

#include "my_vulkan/gpu_allocator.hpp"
#include <functional>

namespace KRV {

class BasePass {
public:
    virtual void AllocateResources(VkDevice device, Utils::GPUAllocator& gpuAllocator) = 0;
    virtual void Init(VkDevice device) = 0;
    virtual void Destroy(VkDevice device) = 0;
    virtual void RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer) = 0;
};

}

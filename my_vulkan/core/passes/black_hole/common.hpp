#pragma once

#include <vulkan/vulkan_core.h>

namespace KRV {

constexpr char const PRECOMPUTED_PHI_TEXTURE_NAME[] = "BlackHolePrecomputePass::PrecomputedPhiTexture";
constexpr char const PRECOMPUTED_ACCR_DISK_DATA_TEXTURE_NAME[] = "BlackHolePrecomputePass::PrecomputedAccrDiskDataTexture";

constexpr VkTransformMatrixKHR const blasTransformMatrices[] = {
    {
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 0.0F, -1.0F, -1.3F,
        0.0F, 1.0F, 0.0F, 0.0F
    }
};

}
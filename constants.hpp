#pragma once

#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#define VK_CALL(call) \
    {\
        auto ret = call;\
        if (ret != VK_SUCCESS) {\
            throw std::runtime_error("Vulkan is dead");\
        }\
    }

namespace KRV {

// Relates to window/swapchain info
constexpr uint32_t WINDOW_SIZE_WIDTH = 800U;
constexpr float WINDOW_SIZE_WIDTH_F = static_cast<float>(WINDOW_SIZE_WIDTH);
constexpr uint32_t WINDOW_SIZE_HEIGHT = 800U;
constexpr float WINDOW_SIZE_HEIGHT_F = static_cast<float>(WINDOW_SIZE_HEIGHT);

// vendorID
constexpr uint32_t AMD_VENDOR_ID = 0x1002;
constexpr uint32_t NVIDIA_VENDOR_ID = 0x10DE;
constexpr uint32_t INTEL_VENDOR_ID = 0x8086;

// Usefull constant variables
constexpr float ZERO_FLOAT_ARRAY[4] = {0.0F, 0.0F, 0.0F, 0.0F};
constexpr VkDeviceSize ZERO_ULL_ARRAY[4] = {0ULL, 0ULL, 0ULL, 0ULL};

namespace STANDART_CLEAR_VALUE {
namespace COLOR {

constexpr VkClearValue FLOAT = {
    .color = {
        .float32 = {0.0F, 0.0F, 0.0F, 0.0F}
    }
};

constexpr VkClearValue INT = {
    .color = {
        .int32 = {0, 0, 0, 0}
    }
};

constexpr VkClearValue UINT = {
    .color = {
        .uint32 = {0U, 0U, 0U, 0U}
    }
};

}

constexpr VkClearValue DEPTH_STENCIL = {
    .depthStencil = {
        .depth = 1.0F,
        .stencil = 0U
    }
};
}

constexpr float ONE_FLOAT = 1.0F;
constexpr float ZERO_FLOAT = 0.0F;

}
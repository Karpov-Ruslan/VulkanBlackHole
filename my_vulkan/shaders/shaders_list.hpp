#pragma once

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace KRV::Utils {

enum class SHADER_LIST_ID {
    BLACK_HOLE_COMP
};

const std::unordered_map<SHADER_LIST_ID, std::vector<uint32_t>> shaderList = {
    {
        SHADER_LIST_ID::BLACK_HOLE_COMP,
        #include <black_hole.comp.spv>
    }
};

VkShaderModule GetShaderModule(VkDevice device, SHADER_LIST_ID id);

}
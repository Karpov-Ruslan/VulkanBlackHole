#include "shaders_list.hpp"
#include <constants.hpp>
#include "my_vulkan/vulkan_functions.hpp"

namespace KRV::Utils {

const std::unordered_map<SHADER_LIST_ID, std::vector<uint32_t>> shaderList = {
    {
        SHADER_LIST_ID::BLACK_HOLE_RAY_MARCHING_RK4_COMP,
        #include <black_hole_ray_marching_rk4.comp.spv>
    },
    {
        SHADER_LIST_ID::BLACK_HOLE_RAY_MARCHING_RK2_COMP,
        #include <black_hole_ray_marching_rk2.comp.spv>
    },
    {
        SHADER_LIST_ID::BLACK_HOLE_RAY_MARCHING_RK1_COMP,
        #include <black_hole_ray_marching_rk1.comp.spv>
    },
    {
        SHADER_LIST_ID::BLACK_HOLE_PRECOMPUTED_COMP,
        #include <black_hole_precomputed.comp.spv>
    },
    {
        SHADER_LIST_ID::BLACK_HOLE_PRECOMPUTE_PRECOMPUTED_TEXTURE_COMP,
        #include <black_hole_precompute_precomputed_texture.comp.spv>
    }
};

ShaderModule::ShaderModule(VkDevice device, SHADER_LIST_ID id) : device(device) {
    auto const &shaderData = shaderList.at(id);

    VkShaderModuleCreateInfo const shaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .codeSize = shaderData.size() * sizeof(uint32_t),
        .pCode = shaderData.data()
    };

    VK_CALL(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule));
}

ShaderModule::~ShaderModule() {
    vkDestroyShaderModule(device, shaderModule, nullptr);
}

}
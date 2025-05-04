#include "shaders_list.hpp"
#include <constants.hpp>

namespace KRV::Utils {

VkShaderModule GetShaderModule(VkDevice device, SHADER_LIST_ID id) {
    const auto& shaderData = shaderList.at(id);

    const VkShaderModuleCreateInfo shaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .codeSize = shaderData.size() * sizeof(uint32_t),
        .pCode = shaderData.data()
    };

    VkShaderModule shaderModule = VK_NULL_HANDLE;

    VK_CALL(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule));

    return shaderModule;
}

}
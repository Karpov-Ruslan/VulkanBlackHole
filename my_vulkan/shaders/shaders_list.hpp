#pragma once

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace KRV::Utils {

enum class SHADER_LIST_ID {
    BLACK_HOLE_COMP
};

class ShaderModule final {
public:
    ShaderModule(VkDevice device, SHADER_LIST_ID id);

    // Force RVO
    ShaderModule(ShaderModule const &) = delete;
    ShaderModule& operator=(ShaderModule const &) = delete;
    ShaderModule(ShaderModule &&) = delete;
    ShaderModule& operator=(ShaderModule &&) = delete;

    ~ShaderModule();

    operator VkShaderModule() {return shaderModule;}

private:
    VkDevice device = VK_NULL_HANDLE;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
};

}
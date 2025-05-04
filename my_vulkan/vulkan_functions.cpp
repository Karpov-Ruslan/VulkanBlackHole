#include "vulkan_functions.hpp"
#include <stdexcept>

namespace KRV {

void LoadVulkanGlobalFunctions() {
#define X(name) if (name = reinterpret_cast<decltype(name)>(vkGetInstanceProcAddr(nullptr, #name)); name == nullptr) {throw std::runtime_error("Cannot Load Vulkan Function: " + #name);}
#include "vulkan_functions/global.hpp"
#undef X
}

void LoadVulkanInstanceFunctions(VkInstance instance) {
#define X(name) if (name = reinterpret_cast<decltype(name)>(vkGetInstanceProcAddr(instance, #name)); name == nullptr) {throw std::runtime_error("Cannot Load Vulkan Function: " + #name);}
#include "vulkan_functions/instance.hpp"
#undef X
}

void LoadVulkanDeviceFunctions(VkDevice device) {
#define X(name) if (name = reinterpret_cast<decltype(name)>(vkGetDeviceProcAddr(device, #name)); name == nullptr) {throw std::runtime_error("Cannot Load Vulkan Function: " + #name);}
#include "vulkan_functions/device.hpp"
#undef X
}

}
#include "vulkan_functions.hpp"
#include <stdexcept>
#include <format>

namespace KRV {

void LoadVulkanGlobalFunctions() {
// Load Vulkan Dynamic Library (because of performance)
HINSTANCE hinstance = LoadLibrary(TEXT("vulkan-1.dll"));
if (!hinstance) {
    throw std::runtime_error("Vulkan Library didn't loaded");
}

vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(hinstance, "vkGetInstanceProcAddr"));
if (!vkGetInstanceProcAddr) {
    throw std::runtime_error("vkGetInstanceProcAddr didn't loaded");
}

#define X(name) if (name = reinterpret_cast<decltype(name)>(vkGetInstanceProcAddr(nullptr, #name)); name == nullptr) {throw std::runtime_error(std::format("Cannot Load Global Vulkan Function: {}", #name));}
#include "vulkan_functions/global.in"
#undef X
}

void LoadVulkanInstanceFunctions(VkInstance instance) {
#define X(name) if (name = reinterpret_cast<decltype(name)>(vkGetInstanceProcAddr(instance, #name)); name == nullptr) {throw std::runtime_error(std::format("Cannot Load Instance Vulkan Function: {}", #name));}
#include "vulkan_functions/instance.in"
#undef X
}

void LoadVulkanDeviceFunctions(VkDevice device) {
#define X(name) if (name = reinterpret_cast<decltype(name)>(vkGetDeviceProcAddr(device, #name)); name == nullptr) {throw std::runtime_error(std::format("Cannot Load Device Vulkan Function: {}", #name));}
#include "vulkan_functions/device.in"
#undef X
}

}
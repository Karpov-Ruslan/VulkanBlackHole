#include <vulkan/vulkan_core.h>

#include <windows.h>
#include <vulkan/vulkan_win32.h>

#define X(name) inline PFN_##name name = nullptr;
inline PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
#include "vulkan_functions/global.in"
#include "vulkan_functions/instance.in"
#include "vulkan_functions/device.in"
#undef X

namespace KRV {

void LoadVulkanGlobalFunctions();
void LoadVulkanInstanceFunctions(VkInstance instance);
void LoadVulkanDeviceFunctions(VkDevice device);

}
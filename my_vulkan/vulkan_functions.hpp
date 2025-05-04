#include <vulkan/vulkan_core.h>

#define X(name) PFN_##name name;
#include "vulkan_functions/global.hpp"
#include "vulkan_functions/instance.hpp"
#include "vulkan_functions/device.hpp"
#undef X

namespace KRV {

void LoadVulkanGlobalFunctions();
void LoadVulkanInstanceFunctions(VkInstance instance);
void LoadVulkanDeviceFunctions(VkDevice device);

}
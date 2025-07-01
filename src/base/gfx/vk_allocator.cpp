#include "../../../headers/base/gfx/vk_allocator.h"


#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0 
#include <vk_mem_alloc.h>

VulkanAllocator::VulkanAllocator(VulkanInstance& instanceObj, VulkanDevice& deviceObj) : _instanceObj{instanceObj}, _deviceObj{deviceObj}
{
	VmaAllocatorCreateInfo createInfo{};
	createInfo.vulkanApiVersion = API_VERSION;
	createInfo.physicalDevice = deviceObj.GetPhysicalDevice();
	createInfo.device = deviceObj.GetDevice();
	createInfo.instance = instanceObj.GetInstance();
	createInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	// Means that accessible only from the one thread or synchrnized by the user ... for BDA

	VmaVulkanFunctions vulkanFuncs{};
	createInfo.pVulkanFunctions = &vulkanFuncs;
	vmaImportVulkanFunctionsFromVolk(&createInfo, &vulkanFuncs);

	vmaCreateAllocator(&createInfo, &_allocator);
}

// Object would be destroyed after all buffers for sure so destructor is fine
void VulkanAllocator::Cleanup()
{
	vmaDestroyAllocator(_allocator);
}
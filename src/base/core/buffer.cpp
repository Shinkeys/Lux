#include "../../../headers/base/core/buffer.h"
#include "../../../headers/base/gfx/vk_base.h"
#include "../../../headers/util/gfx/vk_defines.h"

BufferManager::BufferManager(VulkanBase& vulkanBase) : _vulkanBase{vulkanBase}
{
	VulkanDevice& deviceObj = _vulkanBase.GetVulkanDeviceObj();
	VulkanInstance& instanceObj = _vulkanBase.GetVulkanInstanceObj();
	VulkanAllocator& allocatorObj = _vulkanBase.GetAllocatorObj();

	VmaAllocatorCreateInfo createInfo{};
	createInfo.vulkanApiVersion = API_VERSION;
	createInfo.physicalDevice = deviceObj.GetPhysicalDevice();
	createInfo.device = deviceObj.GetDevice();
	createInfo.instance = instanceObj.GetInstance();
	createInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	// Means that accessible only from the one thread or synchronized by the user ... for BDA

	VmaVulkanFunctions vulkanFuncs{};
	createInfo.pVulkanFunctions = &vulkanFuncs;
	vmaImportVulkanFunctionsFromVolk(&createInfo, &vulkanFuncs);

	vmaCreateAllocator(&createInfo, &allocatorObj.GetAllocatorHandle());
}



std::unique_ptr<Buffer>	 BufferManager::CreateBuffer(const BufferSpecification& spec)	const
{
	return std::make_unique<VulkanBuffer>(spec, _vulkanBase.GetVulkanDeviceObj(), _vulkanBase.GetAllocatorObj(), _vulkanBase.GetFrameObj()); 
}
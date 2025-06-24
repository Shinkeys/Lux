#include "../../../headers/base/gfx/vk_buffer.h"


#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0 
#include <vk_mem_alloc.h>
// For volk compatibility 


VulkanBuffer::VulkanBuffer(VulkanInstance& instanceObj, VulkanDevice& deviceObj) : _instanceObj{instanceObj}, _deviceObj{deviceObj}
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

MeshBuffers& VulkanBuffer::CreateMeshBuffers(const MeshVertexBufferCreateDesc& vertexDesc, const MeshIndexBufferCreateDesc& indexDesc, EntityIndex handleIndex)
{
	MeshBuffers meshBuffers(_allocator);

	meshBuffers.CreateBuffers(_deviceObj.GetDevice(), vertexDesc, indexDesc);
	// Copy constructors are not allowed in mesh buffers
	auto it = _meshBuffers.emplace(handleIndex, std::move(meshBuffers));

	return it.first->second;

}

const MeshBuffers* VulkanBuffer::GetMeshBuffers(EntityIndex handleIndex) const
{
	auto it = _meshBuffers.find(handleIndex);

	if (it == _meshBuffers.end())
		return nullptr;

	return &it->second;
}

const UniformBuffer* VulkanBuffer::GetUniformBuffer(EntityIndex handleIndex) const
{
	auto it = _uniformBuffers.find(handleIndex);

	if (it == _uniformBuffers.end())
		return nullptr;

	return &it->second;
}

void VulkanBuffer::Cleanup()
{
	for (auto& [index, meshBuffers] : _meshBuffers)
	{
		meshBuffers.Cleanup();
	}

	for (auto& [index, uniformBuffer] : _uniformBuffers)
	{
		uniformBuffer.Cleanup();
	}

	vmaDestroyAllocator(_allocator);
}
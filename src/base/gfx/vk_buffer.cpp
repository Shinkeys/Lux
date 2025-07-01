#include "../../../headers/base/gfx/vk_buffer.h"
#include "../../../headers/base/gfx/vk_allocator.h"


// For volk compatibility 


VulkanBuffer::VulkanBuffer(VulkanInstance& instanceObj, VulkanDevice& deviceObj, VulkanAllocator& allocator) : _instanceObj{instanceObj}, _deviceObj{deviceObj}, _allocatorObj{allocator}
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
	
	vmaCreateAllocator(&createInfo, &_allocatorObj.GetAllocatorHandle());
}

MeshBuffers& VulkanBuffer::CreateMeshBuffers(const MeshVertexBufferCreateDesc& vertexDesc, const MeshIndexBufferCreateDesc& indexDesc, EntityIndex handleIndex)
{
	MeshBuffers meshBuffers(_allocatorObj.GetAllocatorHandle());

	meshBuffers.CreateBuffers(_deviceObj.GetDevice(), vertexDesc, indexDesc);
	// Copy constructors are not allowed in mesh buffers
	auto it = _meshBuffers.emplace(handleIndex, std::move(meshBuffers));

	return it.first->second;
}


StagingPair VulkanBuffer::CreateStagingBuffer(void* data, size_t size)
{
	StorageBuffer stagingBuffer(_allocatorObj.GetAllocatorHandle());

	stagingBuffer.CreateBuffer(_deviceObj.GetDevice(), data, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	const i32 availableIndex = _stagingBufferAvailableIndex;
	++_stagingBufferAvailableIndex;

	auto it = _stagingBuffers.emplace(availableIndex, std::move(stagingBuffer));


	StagingPair pair;
	pair.buffer = &it.first->second;
	pair.index = availableIndex;

	return pair;
}

const MeshBuffers* VulkanBuffer::GetMeshBuffers(EntityIndex handleIndex) const
{
	auto it = _meshBuffers.find(handleIndex);

	if (it == _meshBuffers.end())
		return nullptr;

	return &it->second;
}

const StorageBuffer* VulkanBuffer::GetUniformBuffer(EntityIndex handleIndex) const
{
	auto it = _uniformBuffers.find(handleIndex);

	if (it == _uniformBuffers.end())
		return nullptr;

	return &it->second;
}

const StorageBuffer* VulkanBuffer::GetSsboBuffer(BufferIndex handleIndex) const
{
	auto it = _ssboBuffers.find(handleIndex);

	if (it == _ssboBuffers.end())
		return nullptr;

	return &it->second;
}

void VulkanBuffer::DeleteStagingBuffer(BufferIndex handleIndex)
{
	if (auto it = _stagingBuffers.find(handleIndex); it != _stagingBuffers.end())
	{
		vmaDestroyBuffer(_allocatorObj.GetAllocatorHandle(), it->second.GetVkBuffer(), it->second.GetAllocation());

		_stagingBuffers.erase(it);
	}
	else
		std::cout << "Unable to destroy buffer by index: " << handleIndex << " it's not found\n";
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

	for (auto& [index, ssboBuffer] : _ssboBuffers)
	{
		ssboBuffer.Cleanup();
	}

	for (auto& [index, stagingBuffer] : _stagingBuffers)
	{
		stagingBuffer.Cleanup();
	}
}
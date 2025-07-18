#include "../../../headers/base/gfx/vk_buffer.h"
#include "../../../headers/base/gfx/vk_allocator.h"
#include "../../../headers/base/gfx/vk_deleter.h"


// For volk compatibility 

/*
 ___ ___         __ __                     __           ___   ___
|   |   |.--.--.|  |  |--.---.-.-----.    |  |--.--.--.'  _|.'  _|.-----.----.
|   |   ||  |  ||  |    <|  _  |     |    |  _  |  |  |   _||   _||  -__|   _|
 \_____/ |_____||__|__|__|___._|__|__|    |_____|_____|__|  |__|  |_____|__| 
 */
VulkanBuffer::VulkanBuffer(VulkanInstance& instanceObj, VulkanDevice& deviceObj, VulkanAllocator& allocator) : _instanceObj{instanceObj}, _deviceObj{deviceObj}, _allocatorObj{allocator}
{
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
	
	vmaCreateAllocator(&createInfo, &_allocatorObj.GetAllocatorHandle());
}

MeshBuffers& VulkanBuffer::CreateMeshBuffers(const MeshVertexBufferCreateDesc& vertexDesc, const MeshIndexBufferCreateDesc& indexDesc, EntityIndex handleIndex)
{
	MeshBuffers meshBuffers	(_allocatorObj.GetAllocatorHandle());

	meshBuffers.CreateBuffers(_deviceObj.GetDevice(), vertexDesc, indexDesc);
	// Copy constructors are not allowed in mesh buffers
	if (_meshBuffers.find(handleIndex) == _meshBuffers.end())
	{
		std::vector<MeshBuffers> vec;
		vec.emplace_back(std::move(meshBuffers));
		auto it = _meshBuffers.emplace(handleIndex,  std::move(vec) );
		return it.first->second.back();
	}
	else
	{
		auto& emplacedBuffer = _meshBuffers[handleIndex].emplace_back(std::move(meshBuffers));
		return emplacedBuffer;
	}
}


StagingPair VulkanBuffer::CreateStagingBuffer(void* data, size_t size)
{
	StorageBuffer stagingBuffer(_allocatorObj.GetAllocatorHandle());

	stagingBuffer.CreateBuffer(_deviceObj.GetDevice(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stagingBuffer.UpdateBuffer(data, size);

	const i32 availableIndex = _stagingBufferAvailableIndex;
	++_stagingBufferAvailableIndex;

	auto it = _stagingBuffers.emplace(availableIndex, std::move(stagingBuffer));


	StagingPair pair;
	pair.buffer = &it.first->second;
	pair.index = availableIndex;

	return pair;
}

const std::vector<MeshBuffers>* VulkanBuffer::GetMeshBuffers(EntityIndex handleIndex) const
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
		for(auto& buffer : meshBuffers)
			buffer.Cleanup();
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

UBOPair VulkanBuffer::CreateUniformBuffer(size_t size)
{
	StorageBuffer uniformBuffers(_allocatorObj.GetAllocatorHandle());

	uniformBuffers.CreateBuffer(_deviceObj.GetDevice(), size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	const i32 availableIndex = _uniformBufferAvailableIndex;
	++_uniformBufferAvailableIndex;


	auto it = _uniformBuffers.emplace(availableIndex, std::move(uniformBuffers));
	

	UBOPair pair;
	pair.address = it.first->second.GetDeviceAddress();
	pair.index = availableIndex;

	return pair;
}


SSBOPair VulkanBuffer::CreateSSBOBuffer(size_t size)
{
	StorageBuffer ssboBuffer(_allocatorObj.GetAllocatorHandle());

	ssboBuffer.CreateBuffer(_deviceObj.GetDevice(), size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	// Copy constructors are not allowed in mesh buffers

	const i32 availableIndex = _ssboBufferAvailableIndex;
	++_ssboBufferAvailableIndex;

	auto it = _ssboBuffers.emplace(availableIndex, std::move(ssboBuffer));

	SSBOPair pair;
	pair.address = it.first->second.GetDeviceAddress();
	pair.index = availableIndex;

	return pair;
}


/*
_______               __         ______         ___   ___
|   |   |.-----.-----.|  |--.    |   __ \.--.--.'  _|.'  _|.-----.----.-----.
|       ||  -__|__ --||     |    |   __ <|  |  |   _||   _||  -__|   _|__ --|
|__|_|__||_____|_____||__|__|    |______/|_____|__|  |__|  |_____|__| |_____|
																			 */


void MeshBuffers::FillBuffers(const Vertex* vertexData, const u32* indexData, size_t vertexDataSize, size_t indexDataSize)
{
	if (vertexData)
	{
		void* mappedData = nullptr;
		const void* passedData = static_cast<const void*>(vertexData);
		VK_CHECK(vmaMapMemory(_allocator, _vertexAllocation, &mappedData));
		memcpy(mappedData, passedData, (vertexDataSize * sizeof(Vertex)));
		vmaUnmapMemory(_allocator, _vertexAllocation);
	}
	if (indexData)
	{
		void* mappedData = nullptr;
		const void* passedData = static_cast<const void*>(indexData);
		VK_CHECK(vmaMapMemory(_allocator, _indexAllocation, &mappedData));
		memcpy(mappedData, passedData, (indexDataSize * sizeof(u32)));
		vmaUnmapMemory(_allocator, _indexAllocation);
	}
}

void MeshBuffers::CreateBuffers(VkDevice device, const MeshVertexBufferCreateDesc& vertexDesc, const MeshIndexBufferCreateDesc& indexDesc)
{
	VkBufferCreateInfo vertexBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	vertexBufferInfo.size = vertexDesc.elementsCount * sizeof(Vertex);
	vertexBufferInfo.usage = vertexDesc.bufferUsage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; // FOR BDA
	vertexBufferInfo.sharingMode = vertexDesc.bufferSharingMode;

	VmaAllocationCreateInfo vertexAllocInfo{};
	vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	vertexAllocInfo.requiredFlags = vertexDesc.bufferMemoryPropertyFlags;
	vertexAllocInfo.priority = 1.0f;
	vertexAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

	VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vertexAllocInfo, &_vertexBuffer, &_vertexAllocation, nullptr));
	FillVertexBufferAddress(device);

	// Index buffer
	VkBufferCreateInfo indexBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	indexBufferInfo.size = indexDesc.elementsCount * sizeof(u32);
	indexBufferInfo.usage = indexDesc.bufferUsage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; // FOR BDA
	indexBufferInfo.sharingMode = indexDesc.bufferSharingMode;

	VmaAllocationCreateInfo indexAllocInfo{};
	indexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	indexAllocInfo.requiredFlags = indexDesc.bufferMemoryPropertyFlags;
	indexAllocInfo.priority = 1.0f;
	indexAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

	VK_CHECK(vmaCreateBuffer(_allocator, &indexBufferInfo, &indexAllocInfo, &_indexBuffer, &_indexAllocation, nullptr));

	FillBuffers(vertexDesc.vertexPtr, indexDesc.indicesPtr, static_cast<size_t>(vertexDesc.elementsCount), static_cast<size_t>(indexDesc.elementsCount));
}

void MeshBuffers::FillVertexBufferAddress(VkDevice device)
{
	VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	addressInfo.buffer = _vertexBuffer;
	_bufferAddress = vkGetBufferDeviceAddress(device, &addressInfo);
}

void MeshBuffers::Cleanup()
{
	VulkanDeleter::SubmitObjectDesctruction([this](){
		vmaDestroyBuffer(_allocator, _vertexBuffer, _vertexAllocation);
		vmaDestroyBuffer(_allocator, _indexBuffer, _indexAllocation);
	});
}



/*
 _______ __                                     ______         ___   ___
|     __|  |_.-----.----.---.-.-----.-----.    |   __ \.--.--.'  _|.'  _|.-----.----.
|__     |   _|  _  |   _|  _  |  _  |  -__|    |   __ <|  |  |   _||   _||  -__|   _|
|_______|____|_____|__| |___._|___  |_____|    |______/|_____|__|  |__|  |_____|__|
							  |_____|                                               */

void StorageBuffer::FillBufferAddress(VkDevice device)
{
	VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	addressInfo.buffer = _buffer;
	_bufferAddress = vkGetBufferDeviceAddress(device, &addressInfo);
}


void StorageBuffer::CreateBuffer(VkDevice device, size_t size, VkBufferUsageFlags usage)
{
	VkBufferCreateInfo uniformBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	uniformBufferInfo.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	uniformBufferInfo.size = size;
	uniformBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	allocInfo.priority = 1.0f;
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

	VK_CHECK(vmaCreateBuffer(_allocator, &uniformBufferInfo, &allocInfo, &_buffer, &_allocation, nullptr));

	FillBufferAddress(device);
}

void StorageBuffer::Cleanup()
{
	VmaAllocator alloc = _allocator;
	VmaAllocation allocation = _allocation;
	VkBuffer buff = _buffer;
	void* mappedData = _mappedData;


	VulkanDeleter::SubmitObjectDesctruction([alloc, allocation, buff, mappedData]() {
		if (mappedData != nullptr)
			vmaUnmapMemory(alloc, allocation);

		vmaDestroyBuffer(alloc, buff, allocation);
	});
}
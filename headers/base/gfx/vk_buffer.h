#pragma once
#include "vk_frame.h"
#include "vk_allocator.h"
#include "../../asset/asset_manager.h"


struct MeshVertexBufferCreateDesc
{
	const Vertex* vertexPtr;
	u32 elementsCount;

	VkBufferUsageFlags bufferUsage;
	VkMemoryPropertyFlags bufferMemoryPropertyFlags;
	VkSharingMode bufferSharingMode; 
};

struct MeshIndexBufferCreateDesc
{
	const u32* indicesPtr;
	u32 elementsCount;

	VkBufferUsageFlags bufferUsage;
	VkMemoryPropertyFlags bufferMemoryPropertyFlags;
	VkSharingMode bufferSharingMode;
};

/**
* @brief Doesn't work for copy, you can only MOVE it
*/
class MeshBuffers
{
private:
	const VmaAllocator& _allocator;

	VkBuffer _vertexBuffer{ VK_NULL_HANDLE };
	VkBuffer _indexBuffer{ VK_NULL_HANDLE };
	VmaAllocation _vertexAllocation{ VK_NULL_HANDLE };
	VmaAllocation _indexAllocation{ VK_NULL_HANDLE };
	VkDeviceAddress _bufferAddress{ 0 };

	void FillVertexBufferAddress(VkDevice device)
	{
		VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		addressInfo.buffer = _vertexBuffer;
		_bufferAddress = vkGetBufferDeviceAddress(device, &addressInfo);
	}
	/**
	* @brief Fill buffers with the data
	* @param vertex data ptr
	* @param index  data ptr
	* @param vertex data size
	* @param index  data size
	* size NOT in bytes
	*/
	void FillBuffers(const Vertex* vertexData, const u32* indexData, size_t vertexDataSize, size_t indexDataSize)
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

public:
	VkDeviceAddress GetDeviceAddress() const { return _bufferAddress; }
	VkBuffer GetVkVertexBuffer()	   const { return _vertexBuffer; }
	VkBuffer GetVkIndexBuffer()        const { return _indexBuffer; }


	MeshBuffers() = delete;
	MeshBuffers(VmaAllocator& allocator) : _allocator{ allocator }{}
	~MeshBuffers() = default;
	MeshBuffers(const MeshBuffers& other) = delete;
	MeshBuffers& operator=(const MeshBuffers& other) = delete;

	MeshBuffers(MeshBuffers&& other) noexcept : _allocator{ other._allocator }
	{
		_vertexAllocation = other._vertexAllocation;
		_indexAllocation = other._indexAllocation;
		_vertexBuffer = other._vertexBuffer;
		_indexBuffer = other._indexBuffer;
		_bufferAddress = other._bufferAddress;

		other._vertexBuffer = VK_NULL_HANDLE;
		other._indexBuffer = VK_NULL_HANDLE;
		other._vertexAllocation = nullptr;
		other._indexAllocation = nullptr;
		other._bufferAddress = 0;
	}

	MeshBuffers& operator=(MeshBuffers&& other) noexcept
	{
		if (this != &other)
		{
			vmaDestroyBuffer(_allocator, _vertexBuffer, _vertexAllocation);
			vmaDestroyBuffer(_allocator, _indexBuffer, _indexAllocation);
			_vertexAllocation = other._vertexAllocation;
			_indexAllocation = other._indexAllocation;
			_vertexBuffer = other._vertexBuffer;
			_indexBuffer = other._indexBuffer;
			_bufferAddress = other._bufferAddress;

			other._vertexBuffer = VK_NULL_HANDLE;
			other._indexBuffer = VK_NULL_HANDLE;
			other._vertexAllocation = nullptr;
			other._indexAllocation = nullptr;
			other._bufferAddress = 0;
		}


		return *this;
	}

	void CreateBuffers(VkDevice device, const MeshVertexBufferCreateDesc& vertexDesc, const MeshIndexBufferCreateDesc& indexDesc)
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
	// Can't use destructor as need to ensure that the object would be destroyed earlier than allocator
	/**
	* @brief DON'T CALL IT ANYWHERE EXCEPT VULKANBUFFER CLASS!!!
	*/
	void Cleanup()
	{
		vmaDestroyBuffer(_allocator, _vertexBuffer, _vertexAllocation);
		vmaDestroyBuffer(_allocator, _indexBuffer, _indexAllocation);
	}
};

class StorageBuffer
{
private:
	const VmaAllocator& _allocator;

	VkBuffer _buffer{ VK_NULL_HANDLE };
	VmaAllocation _allocation{ VK_NULL_HANDLE };
	VkDeviceAddress _bufferAddress{ 0 };
	void* _mappedData{ nullptr };

	void FillBufferAddress(VkDevice device)
	{
		VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		addressInfo.buffer = _buffer;
		_bufferAddress = vkGetBufferDeviceAddress(device, &addressInfo);
	}
public:
	VkDeviceAddress GetDeviceAddress() const { return _bufferAddress; }
	VkBuffer GetVkBuffer()	           const { return _buffer; }
	VmaAllocation GetAllocation()      const { return _allocation; }

	StorageBuffer() = delete;
	StorageBuffer(VmaAllocator& allocator) : _allocator{ allocator } {}
	~StorageBuffer() = default;
	StorageBuffer(const StorageBuffer& other) = delete;
	StorageBuffer& operator=(const StorageBuffer& other) = delete;

	StorageBuffer(StorageBuffer&& other) noexcept : _allocator{ other._allocator }
	{
		_allocation = other._allocation;
		_buffer = other._buffer;
		_bufferAddress = other._bufferAddress;
		_mappedData = other._mappedData;

		other._allocation = nullptr;
		other._buffer = VK_NULL_HANDLE;
		other._bufferAddress = 0;
		other._mappedData = nullptr;
	}

	StorageBuffer& operator=(StorageBuffer&& other) noexcept
	{
		if (this != &other)
		{
			vmaDestroyBuffer(_allocator, _buffer, _allocation);
			_allocation = other._allocation;
			_buffer = other._buffer;
			_bufferAddress = other._bufferAddress;

			other._allocation = nullptr;
			other._buffer = VK_NULL_HANDLE;
			other._bufferAddress = 0;
		}
		return *this;
	}
	
	template<typename T>
	void UpdateBuffer(const T* data, size_t size)
	{
		if (_mappedData == nullptr)
		{
			VK_CHECK(vmaMapMemory(_allocator, _allocation, &_mappedData));
		}
		const void* passedData = static_cast<const void*>(data);
		memcpy(_mappedData, passedData, size);

		// UNMAP LATER!!!!!!
	}

	template<typename T>
	void CreateBuffer(VkDevice device, const T* data, size_t size, VkBufferUsageFlags usage)
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
		UpdateBuffer(data, size);
	}	

	void Cleanup()
	{
		vmaUnmapMemory(_allocator, _allocation);
		vmaDestroyBuffer(_allocator, _buffer, _allocation);
	}
};

// Purpose: main class to handle buffers logic.
class VulkanBuffer
{
private:
	VulkanInstance& _instanceObj;
	VulkanDevice& _deviceObj;
	VulkanAllocator& _allocatorObj;

	using EntityIndex = u32;
	using BufferIndex = i32;
	// TO REFACTOR
	std::unordered_map<EntityIndex, MeshBuffers> _meshBuffers;
	std::unordered_map<EntityIndex, StorageBuffer> _uniformBuffers;

	i32 _stagingBufferAvailableIndex{ 1 };
	i32 _ssboBufferAvailableIndex{ 1 };
	std::unordered_map<BufferIndex, StorageBuffer> _ssboBuffers;
	std::unordered_map<BufferIndex, StorageBuffer> _stagingBuffers;
public:
	void Cleanup();
	// Mesh
	MeshBuffers& CreateMeshBuffers(const MeshVertexBufferCreateDesc& vertexDesc, const MeshIndexBufferCreateDesc& indexDesc, EntityIndex handleIndex);
	const MeshBuffers* GetMeshBuffers(EntityIndex handleIndex) const;
	
	// Uniform
	template<typename T>
	StorageBuffer& CreateUniformBuffer(const T& data, size_t size, EntityIndex handleIndex);
	template<typename T>
	void UpdateUniformBuffer(const T& data, size_t size, EntityIndex handleIndex);
	const StorageBuffer* GetUniformBuffer(EntityIndex handleIndex) const;

	// SSBO
	const StorageBuffer* GetSsboBuffer(BufferIndex handleIndex) const;
	/*template<typename T>
	SSBOPair CreateSSBOBuffer(const T& data, size_t size);*/
	template<typename T>
	SSBOPair CreateSSBOBuffer(const std::vector<T>& vector);

	// Staging
	StagingPair CreateStagingBuffer(void* data, size_t size);
	void DeleteStagingBuffer(BufferIndex handleIndex);

	VulkanBuffer() = delete;
	~VulkanBuffer() = default;
	VulkanBuffer(VulkanInstance& instanceObj, VulkanDevice& deviceObj, VulkanAllocator& allocator);


	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer(VulkanBuffer&&) = delete;
	VulkanBuffer& operator= (const VulkanBuffer&) = delete;
	VulkanBuffer& operator= (VulkanBuffer&&) = delete;
};

template<typename T>
StorageBuffer& VulkanBuffer::CreateUniformBuffer(const T& data, size_t size, EntityIndex handleIndex)
{
	StorageBuffer uniformBuffers(_allocatorObj.GetAllocatorHandle());

	uniformBuffers.CreateBuffer(_deviceObj.GetDevice(), &data, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	// Copy constructors are not allowed in mesh buffers
	auto it = _uniformBuffers.emplace(handleIndex, std::move(uniformBuffers));

	return it.first->second;
}

//template<typename T>
//SSBOPair VulkanBuffer::CreateSSBOBuffer(const T& data, size_t size)
//{
//	StorageBuffer ssboBuffer(_allocatorObj.GetAllocatorHandle());
//
//	ssboBuffer.CreateBuffer(_deviceObj.GetDevice(), &data, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
//	// Copy constructors are not allowed in mesh buffers
//	
//	const i32 availableIndex = _ssboBufferAvailableIndex;
//	++_ssboBufferAvailableIndex;
//
//	auto it = _ssboBuffers.emplace(availableIndex, std::move(ssboBuffer));
//	
//	SSBOPair pair;
//	pair.address = it.first->second.GetDeviceAddress();
//	pair.index = availableIndex;
//
//	return pair;
//}

template<typename T>
SSBOPair VulkanBuffer::CreateSSBOBuffer(const std::vector<T>& vector)
{
	StorageBuffer ssboBuffer(_allocatorObj.GetAllocatorHandle());

	ssboBuffer.CreateBuffer(_deviceObj.GetDevice(), vector.data(), vector.size() * sizeof(T), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	const i32 availableIndex = _ssboBufferAvailableIndex;
	++_ssboBufferAvailableIndex;

	auto it = _ssboBuffers.emplace(availableIndex, std::move(ssboBuffer));

	SSBOPair pair;
	pair.address = it.first->second.GetDeviceAddress();
	pair.index = availableIndex;

	return pair;
}

template<typename T>
void VulkanBuffer::UpdateUniformBuffer(const T& data, size_t size, EntityIndex handleIndex)
{
	auto it = _uniformBuffers.find(handleIndex);

	if (it == _uniformBuffers.end())
	{
		Logger::Log("Cannot update uniform buffer for entity, it is not found in the storage: " + std::to_string(handleIndex));
		return;
	}

	it->second.UpdateBuffer(&data, size);
}

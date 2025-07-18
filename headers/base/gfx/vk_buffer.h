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

	void FillVertexBufferAddress(VkDevice device);
	/**
	* @brief Fill buffers with the data
	* @param vertex data ptr
	* @param index  data ptr
	* @param vertex data size
	* @param index  data size
	* size NOT in bytes
	*/
	void FillBuffers(const Vertex* vertexData, const u32* indexData, size_t vertexDataSize, size_t indexDataSize);

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

	void CreateBuffers(VkDevice device, const MeshVertexBufferCreateDesc& vertexDesc, const MeshIndexBufferCreateDesc& indexDesc);
	// Can't use destructor as need to ensure that the object would be destroyed earlier than allocator
	/**
	* @brief DON'T CALL IT ANYWHERE EXCEPT VULKANBUFFER CLASS!!!
	*/
	void Cleanup();
};

class StorageBuffer
{
private:
	const VmaAllocator& _allocator;

	VkBuffer _buffer{ VK_NULL_HANDLE };
	VmaAllocation _allocation{ VK_NULL_HANDLE };
	VkDeviceAddress _bufferAddress{ 0 };
	void* _mappedData{ nullptr };

	void FillBufferAddress(VkDevice device);

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

	void CreateBuffer(VkDevice device, size_t size, VkBufferUsageFlags usage);

	void Cleanup();
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
	std::unordered_map<EntityIndex, std::vector<MeshBuffers>> _meshBuffers;
	std::unordered_map<EntityIndex, StorageBuffer> _uniformBuffers;

	i32 _stagingBufferAvailableIndex{ 1 };
	i32 _ssboBufferAvailableIndex{ 1 };
	i32 _uniformBufferAvailableIndex{ 1 };
	std::unordered_map<BufferIndex, StorageBuffer> _ssboBuffers;
	std::unordered_map<BufferIndex, StorageBuffer> _stagingBuffers;
public:
	// Mesh
	MeshBuffers& CreateMeshBuffers(const MeshVertexBufferCreateDesc& vertexDesc, const MeshIndexBufferCreateDesc& indexDesc, EntityIndex handleIndex);
	const std::vector<MeshBuffers>* GetMeshBuffers(EntityIndex handleIndex) const;
	
	// Uniform
	UBOPair CreateUniformBuffer(size_t size);
	template<typename T>
	void UpdateUniformBuffer(const T* data, size_t size, BufferIndex handleIndex);
	const StorageBuffer* GetUniformBuffer(EntityIndex handleIndex) const;

	// SSBO
	const StorageBuffer* GetSsboBuffer(BufferIndex handleIndex) const;
	SSBOPair CreateSSBOBuffer(size_t size);
	template<typename T>
	void UpdateSSBOBuffer(const T* data, size_t size, BufferIndex handleIndex);

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

	void Cleanup();
};


template<typename T>
void VulkanBuffer::UpdateSSBOBuffer(const T* data, size_t size, BufferIndex handleIndex)
{
	auto it = _ssboBuffers.find(handleIndex);

	if (it == _ssboBuffers.end())
	{
		std::cout << "Can't update ssbo buffer by handle: " << handleIndex << " it's empty!\n";
		return;
	}
	
	it->second.UpdateBuffer(data, size);
}


template<typename T>
void VulkanBuffer::UpdateUniformBuffer(const T* data, size_t size, BufferIndex handleIndex)
{
	auto it = _uniformBuffers.find(handleIndex);

	if (it == _uniformBuffers.end())
	{
		Logger::Log("Cannot update uniform buffer for entity, it is not found in the storage: " + std::to_string(handleIndex));
		return;
	}

	it->second.UpdateBuffer(data, size);
}

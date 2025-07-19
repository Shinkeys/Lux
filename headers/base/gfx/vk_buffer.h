#pragma once
#include "../core/buffer.h"



namespace vkconversions
{
	VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage);
	VmaMemoryUsage ToVmaMemoryUsage(MemoryUsage usage);
	VkMemoryPropertyFlags ToVkMemoryPropertyFlags(MemoryProperty flags);
	VmaAllocationCreateFlags ToVmaAllocationCreateFlags(AllocationCreate flags);
	VkSharingMode ToVkSharingMode(SharingMode mode);
}

class VulkanDevice;
class VulkanAllocator;
class VulkanFrame;
// Purpose: main class to handle buffers logic.
class VulkanBuffer : public Buffer
{
private:
	VulkanDevice& _deviceObj;
	VulkanAllocator& _allocatorObj;
	VulkanFrame& _frameObj;

	BufferSpecification _specification;

	VkBuffer _buffer{ VK_NULL_HANDLE };
	VmaAllocation _allocation{ nullptr };

	void* _mappedData{ nullptr };

public:
	u64 GetBufferAddress() const override;

	VulkanBuffer(const BufferSpecification& spec, VulkanDevice& deviceObject, VulkanAllocator& allocatorObj, VulkanFrame& frameObj);
	//VulkanBuffer(const BufferSpecification& spec, VkBuffer buffer, VmaAllocation allocation);

	VulkanBuffer() = delete;
	~VulkanBuffer();
	VulkanBuffer(const VulkanBuffer& other) = delete;
	VulkanBuffer& operator=(const VulkanBuffer& other) = delete;

	VulkanBuffer(VulkanBuffer&& other) noexcept;

	VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

	VkBuffer GetRawBuffer() const { return _buffer; }

	const BufferSpecification& GetSpecification() const override { return _specification; }

	void  UploadData(u64 offset, const void* data, u64 size) override;
};

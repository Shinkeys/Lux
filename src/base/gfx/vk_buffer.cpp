#include "../../../headers/base/gfx/vk_buffer.h"
#include "../../../headers/base/gfx/vk_allocator.h"
#include "../../../headers/base/gfx/vk_device.h"
#include "../../../headers/base/gfx/vk_deleter.h"


// For volk compatibility 

/*
 ___ ___         __ __                     __           ___   ___
|   |   |.--.--.|  |  |--.---.-.-----.    |  |--.--.--.'  _|.'  _|.-----.----.
|   |   ||  |  ||  |    <|  _  |     |    |  _  |  |  |   _||   _||  -__|   _|
 \_____/ |_____||__|__|__|___._|__|__|    |_____|_____|__|  |__|  |_____|__| 
 */
VulkanBuffer::VulkanBuffer(const BufferSpecification& spec, VulkanDevice& deviceObject, VulkanAllocator& allocatorObj, VulkanFrame& frameObj) :
	_specification{spec}, _deviceObj{deviceObject}, _allocatorObj{allocatorObj}, _frameObj{frameObj}
{
	assert(spec.memoryUsage != MemoryUsage::UNKNOWN && spec.usage != BufferUsage::NONE &&
		"Unable to create vulkan buffer, memory usage or buffer usage is empty");

	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = spec.size;
	bufferInfo.usage = vkconversions::ToVkBufferUsage(spec.usage);
	bufferInfo.sharingMode = vkconversions::ToVkSharingMode(spec.sharingMode);

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = vkconversions::ToVmaMemoryUsage(spec.memoryUsage);
	allocInfo.requiredFlags = vkconversions::ToVkMemoryPropertyFlags(spec.memoryProp);
	allocInfo.priority = 1.0f;
	allocInfo.flags = vkconversions::ToVmaAllocationCreateFlags(spec.allocCreate);

	VK_CHECK(vmaCreateBuffer(_allocatorObj.GetAllocatorHandle(), &bufferInfo, &allocInfo, &_buffer, &_allocation, nullptr));

	if (spec.memoryProp & MemoryProperty::HOST_VISIBLE || spec.memoryProp & MemoryProperty::HOST_COHERENT)
		VK_CHECK(vmaMapMemory(_allocatorObj.GetAllocatorHandle(), _allocation, nullptr));
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept : _deviceObj{ other._deviceObj }, _allocatorObj{ other._allocatorObj }, _frameObj{ other._frameObj }
{
	_allocation = other._allocation;
	_buffer = other._buffer;

	other._buffer = VK_NULL_HANDLE;
	other._allocation = nullptr;
}


VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
{
	if (this != &other)
	{
		if (_allocation != nullptr && _buffer != VK_NULL_HANDLE)
			vmaDestroyBuffer(_allocatorObj.GetAllocatorHandle(), _buffer, _allocation);

		_allocation = other._allocation;
		_buffer = other._buffer;

		other._buffer = VK_NULL_HANDLE;
		other._allocation = nullptr;
	}

	return *this;
}


void  VulkanBuffer::UploadData(u64 offset, const void* newData, u64 size)
{
	if (offset > _specification.size)
		return;

	if (_mappedData != nullptr)
	{
		memcpy(static_cast<u8*>(_mappedData) + offset, newData, size);
	}
	else // if buffer GPU only
	{
		VkBuffer stagingBuff;
		VmaAllocation stagingAlloc;

		VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		allocInfo.priority = 1.0f;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VK_CHECK(vmaCreateBuffer(_allocatorObj.GetAllocatorHandle(), &bufferInfo, &allocInfo, &stagingBuff, &stagingAlloc, nullptr));
		
		void* buffData = nullptr;
		VK_CHECK(vmaMapMemory(_allocatorObj.GetAllocatorHandle(), stagingAlloc, &buffData));
		memcpy(buffData, newData, size);
		vmaUnmapMemory(_allocatorObj.GetAllocatorHandle(), stagingAlloc);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = offset;
		copyRegion.size = size;

		vkCmdCopyBuffer(_frameObj.GetCommandBuffer(), stagingBuff, _buffer, 1, &copyRegion);
	}
}

u64 VulkanBuffer::GetBufferAddress() const
{
	VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	addressInfo.buffer = _buffer;
	return vkGetBufferDeviceAddress(_deviceObj.GetDevice(), &addressInfo);
}

VulkanBuffer::~VulkanBuffer()
{
	VmaAllocator allocator = _allocatorObj.GetAllocatorHandle();
	VkBuffer buffer = _buffer;
	VmaAllocation allocation = _allocation;

	if (_mappedData != nullptr)
		vmaUnmapMemory(allocator, _allocation);

	VulkanDeleter::SubmitObjectDesctruction([allocator, buffer, allocation](){
		vmaDestroyBuffer(allocator, buffer, allocation);
	});
}


namespace vkconversions
{
	VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage)
	{
		VkBufferUsageFlags result = 0;

		if (usage & BufferUsage::TRANSFER_SRC)
			result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		if (usage & BufferUsage::TRANSFER_DST)
			result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		if (usage & BufferUsage::UNIFORM_BUFFER)
			result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		if (usage & BufferUsage::STORAGE_BUFFER)
			result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		if (usage & BufferUsage::INDEX_BUFFER)
			result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		if (usage & BufferUsage::VERTEX_BUFFER)
			result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		if (usage & BufferUsage::INDIRECT_BUFFER)
			result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

		if (usage & BufferUsage::SHADER_DEVICE_ADDRESS)
			result |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		if (usage & BufferUsage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY)
			result |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		if (usage & BufferUsage::ACCELERATION_STRUCTURE_STORAGE)
			result |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;

		return result;
	}

	VmaMemoryUsage ToVmaMemoryUsage(MemoryUsage usage)
	{
		switch (usage)
		{
		case MemoryUsage::AUTO:
			return VMA_MEMORY_USAGE_AUTO;

		case MemoryUsage::AUTO_PREFER_DEVICE:
			return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		case MemoryUsage::AUTO_PREFER_HOST:
			return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

		case MemoryUsage::UNKNOWN:
			return VMA_MEMORY_USAGE_UNKNOWN;

		default:
			std::unreachable();
		}
	}

	VkMemoryPropertyFlags ToVkMemoryPropertyFlags(MemoryProperty flags)
	{
		VkMemoryPropertyFlags result = 0;

		if (flags & MemoryProperty::DEVICE_LOCAL)
			result |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if (flags & MemoryProperty::HOST_VISIBLE)
			result |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		if (flags & MemoryProperty::HOST_COHERENT)
			result |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		if (flags & MemoryProperty::HOST_CACHED)
			result |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

		return result;
	}

	VmaAllocationCreateFlags ToVmaAllocationCreateFlags(AllocationCreate flags)
	{
		VmaAllocationCreateFlags result = 0;

		if (flags & AllocationCreate::DEDICATED_MEMORY)
			result |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

		if (flags & AllocationCreate::NEVER_ALLOCATE)
			result |= VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT;

		if (flags & AllocationCreate::MAPPED)
			result |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

		if (flags & AllocationCreate::WITHIN_BUDGET)
			result |= VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;

		if (flags & AllocationCreate::HOST_ACCESS_SEQUENTIAL_WRITE)
			result |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		if (flags & AllocationCreate::HOST_ACCESS_RANDOM)
			result |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

		if (flags & AllocationCreate::STRATEGY_BEST_FIT)
			result |= VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;

		if (flags & AllocationCreate::STRATEGY_FIRST_FIT)
			result |= VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT;

		return result;
	}

	VkSharingMode ToVkSharingMode(SharingMode mode)
	{
		switch (mode)
		{
		case SharingMode::SHARING_EXCLUSIVE: return VK_SHARING_MODE_EXCLUSIVE;

		case SharingMode::SHARING_CONCURRENT: return VK_SHARING_MODE_CONCURRENT;

		default:
			std::unreachable();
		}
	}
}
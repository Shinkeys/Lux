#pragma once
#include "../../util/gfx/vk_types.h"
#include "vk_frame.h"

#include <vk_mem_alloc.h>

class VulkanAllocator
{
private:
	VulkanInstance& _instanceObj;
	VulkanDevice& _deviceObj;

	VmaAllocator _allocator;
public:
	VulkanAllocator() = delete;
	~VulkanAllocator() = default;
	VulkanAllocator(VulkanInstance& instanceObj, VulkanDevice& deviceObj);

	VmaAllocator& GetAllocatorHandle() { return _allocator; }

	VulkanAllocator(const VulkanAllocator&) = delete;
	VulkanAllocator(VulkanAllocator&&) = delete;
	VulkanAllocator& operator= (const VulkanAllocator&) = delete;
	VulkanAllocator& operator= (VulkanAllocator&&) = delete;

	void Cleanup();
};
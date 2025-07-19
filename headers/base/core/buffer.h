#pragma once
#include "../../util/gfx/vk_types.h"
#include "buffer_types.h"


struct BufferSpecification
{
	BufferUsage usage{BufferUsage::NONE};
	SharingMode sharingMode{SharingMode::SHARING_EXCLUSIVE};
	MemoryUsage memoryUsage{ MemoryUsage::UNKNOWN };
	MemoryProperty memoryProp{ MemoryProperty::NONE };
	AllocationCreate allocCreate{ AllocationCreate::NONE };

	size_t size{ 0 }; // bytes
};

class Buffer
{
private:

public:
	virtual ~Buffer() {}

	virtual void  UploadData(u64 offset, const void* data, u64 size) = 0;
	virtual const BufferSpecification& GetSpecification() const = 0;

	virtual u64 GetBufferAddress() const = 0;
};

class VulkanBase;
class BufferManager
{
private:
	VulkanBase& _vulkanBase;

public:

	BufferManager(VulkanBase& vulkanBase);

	std::unique_ptr<Buffer>	 CreateBuffer(const BufferSpecification& spec)	 const;
};
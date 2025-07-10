#pragma once
#include "../../util/util.h"
#include "../../util/gfx/vk_types.h"

// Don't need others for now, at least for vulkan because uses BDA  for buffers/input attachments instead of descriptors
enum class DescriptorType : u8
{
	COMBINED_IMAGE_SAMPLER,
	SAMPLED_IMAGE,
	STORAGE_IMAGE,
};

struct DescriptorSetLayoutBinding
{
	u32              binding;
	DescriptorType   descriptorType;
	u32              descriptorCount;
};

struct DescriptorSpecification
{
	std::vector<DescriptorSetLayoutBinding> bindings;
};


class Image;
class Sampler;
class Descriptor
{
private:

public:
	virtual ~Descriptor() {}

	virtual void Write(u32 dstBinding, u32 dstArrayElem, DescriptorType type, Image* image, Sampler* sampler) = 0;

};

class VulkanBase;
class DescriptorManager
{
private:
	VulkanBase& _vulkanBase;
	VkDescriptorPool _commonPool; // to do a vector

public:
	DescriptorManager(VulkanBase& vulkanBase);

	void Cleanup();

	std::unique_ptr<Descriptor> CreateDescriptorSet(const DescriptorSpecification& spec);
};
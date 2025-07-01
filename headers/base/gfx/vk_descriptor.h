#pragma once
#include "../../util/gfx/vk_types.h"
#include "vk_device.h"


struct DescriptorInfo
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct DescriptorSet
{
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
};

struct DescriptorUpdate
{
	DescriptorSet set{};
	u32 dstBinding{ 0 };
	u32 dstArrayElem{ 0 };
	VkImageView imgView{ VK_NULL_HANDLE };
	VkSampler sampler{ VK_NULL_HANDLE };
};


class VulkanDescriptor
{
private:
	VulkanDevice& _deviceObject;
	VkDescriptorPool _commonPool; // to do a vector
	VkDescriptorSetLayout _bindlessLayout{ VK_NULL_HANDLE };

	std::vector<VkDescriptorSetLayout> _layoutsToDestroy;

	std::queue<DescriptorUpdate> _queueToWriteSets;
public:
	void UpdateBindlessDescriptorSet(const DescriptorUpdate& updateData);
	VkDescriptorSetLayout GetBindlessDescriptorSetLayout() const { return _bindlessLayout; }
	void UpdateSets();
	DescriptorSet CreateDescSet(const DescriptorInfo& info);
	VulkanDescriptor() = delete;
	~VulkanDescriptor() = default;
	VulkanDescriptor(VulkanDevice& deviceObj);


	VulkanDescriptor(const VulkanDescriptor&) = delete;
	VulkanDescriptor(VulkanDescriptor&&) = delete;
	VulkanDescriptor& operator= (const VulkanDescriptor&) = delete;
	VulkanDescriptor& operator= (VulkanDescriptor&&) = delete;

	void Cleanup();
};


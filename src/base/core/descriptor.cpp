#include "../../../headers/base/core/descriptor.h"
#include "../../../headers/base/gfx/vk_descriptor.h"
#include "../../../headers/base/gfx/vk_base.h"

DescriptorManager::DescriptorManager(VulkanBase& vulkanBase) : _vulkanBase{vulkanBase}
{
	u32 descriptorCount = UINT16_MAX * VulkanFrame::FramesInFlight;
	// Create a common desc pool
	std::vector<VkDescriptorPoolSize> poolSizes
	{
			{ VK_DESCRIPTOR_TYPE_SAMPLER,				 descriptorCount },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,			 descriptorCount },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,			 descriptorCount },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		 descriptorCount },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		 descriptorCount },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,		 descriptorCount }
	};		
	
	VkDescriptorPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	createInfo.maxSets = 1000; // some random number
	createInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();
	
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	
	VK_CHECK(vkCreateDescriptorPool(_vulkanBase.GetVulkanDeviceObj().GetDevice(), &createInfo, nullptr, &_commonPool));
}


std::unique_ptr<Descriptor> DescriptorManager::CreateDescriptorSet(const DescriptorSpecification& spec)
{
	return std::make_unique<VulkanDescriptor>(spec, _vulkanBase.GetVulkanDeviceObj(), _commonPool);
}

void DescriptorManager::Cleanup()
{
	vkDestroyDescriptorPool(_vulkanBase.GetVulkanDeviceObj().GetDevice(), _commonPool, nullptr);
}
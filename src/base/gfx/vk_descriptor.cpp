#include "../../../headers/base/gfx/vk_descriptor.h"
#include "../../../headers/base/gfx/vk_frame.h"


VulkanDescriptor::VulkanDescriptor(VulkanDevice& deviceObj) : _deviceObject{deviceObj}
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
	
	VK_CHECK(vkCreateDescriptorPool(_deviceObject.GetDevice(), &createInfo, nullptr, &_commonPool));
}

DescriptorSet VulkanDescriptor::CreateDescSet(const DescriptorInfo& info)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkDescriptorBindingFlags> flags;
	for (const auto& binding : info.bindings)
	{
		VkDescriptorSetLayoutBinding currentBinding{};
		currentBinding.binding = binding.binding;
		currentBinding.descriptorType = binding.descriptorType;
		currentBinding.descriptorCount = binding.descriptorCount;
		currentBinding.stageFlags = VK_SHADER_STAGE_ALL;

		bindings.push_back(currentBinding);
		flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
	bindingFlags.pBindingFlags = flags.data();
	bindingFlags.bindingCount = static_cast<u32>(flags.size());

	VkDescriptorSetLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.pBindings = bindings.data();
	createInfo.bindingCount = static_cast<u32>(info.bindings.size());
	createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	createInfo.pNext = &bindingFlags;

	VK_CHECK(vkCreateDescriptorSetLayout(_deviceObject.GetDevice(), &createInfo, nullptr, &_bindlessLayout));
	
	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pSetLayouts = &_bindlessLayout;
	allocInfo.descriptorSetCount = 1;
	allocInfo.descriptorPool = _commonPool;

	VkDescriptorSet set;
	VK_CHECK(vkAllocateDescriptorSets(_deviceObject.GetDevice(), &allocInfo, &set));

	_layoutsToDestroy.push_back(_bindlessLayout);

	return DescriptorSet(set);
}

void VulkanDescriptor::UpdateBindlessDescriptorSet(const DescriptorUpdate& updateData)
{
	_queueToWriteSets.emplace(updateData);
}

void VulkanDescriptor::UpdateSets()
{
	while (!_queueToWriteSets.empty())
	{
		const DescriptorUpdate& updateData = _queueToWriteSets.front();

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageView = updateData.imgView;
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgInfo.sampler = updateData.sampler;

		VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		writeSet.dstSet = updateData.set.descriptorSet;
		writeSet.dstBinding = updateData.dstBinding;
		writeSet.dstArrayElement = updateData.dstArrayElem;
		writeSet.descriptorCount = 1;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeSet.pImageInfo = &imgInfo;


		vkUpdateDescriptorSets(_deviceObject.GetDevice(), 1, &writeSet, 0, nullptr);
		_queueToWriteSets.pop();
	}

}


void VulkanDescriptor::Cleanup()
{
	for (auto& layout : _layoutsToDestroy)
		vkDestroyDescriptorSetLayout(_deviceObject.GetDevice(), layout, nullptr);

	vkDestroyDescriptorPool(_deviceObject.GetDevice(), _commonPool, nullptr);
}
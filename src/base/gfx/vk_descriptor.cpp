#include "../../../headers/base/gfx/vk_descriptor.h"
#include "../../../headers/base/gfx/vk_deleter.h"
#include "../../../headers/base/gfx/vk_frame.h"
#include "../../../headers/base/gfx/vk_image.h"
#include "../../../headers/base/gfx/raytracing/vk_acceleration_structure.h"

VulkanDescriptor::VulkanDescriptor(const DescriptorSpecification& spec, VulkanDevice& deviceObj, VkDescriptorPool descPool) : 
	_deviceObject{deviceObj}
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkDescriptorBindingFlags> flags;
	for (const auto& binding : spec.bindings)
	{
		VkDescriptorSetLayoutBinding currentBinding{};
		currentBinding.binding = binding.binding;
		currentBinding.descriptorType = ToVkDescriptorType(binding.descriptorType);
		currentBinding.descriptorCount = binding.descriptorCount;
		currentBinding.stageFlags = VK_SHADER_STAGE_ALL; // Doesn't really affect anything so don't care

		bindings.push_back(currentBinding);
		flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
	bindingFlags.pBindingFlags = flags.data();
	bindingFlags.bindingCount = static_cast<u32>(flags.size());

	VkDescriptorSetLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.pBindings = bindings.data();
	createInfo.bindingCount = static_cast<u32>(bindings.size());
	createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	createInfo.pNext = &bindingFlags;

	VK_CHECK(vkCreateDescriptorSetLayout(_deviceObject.GetDevice(), &createInfo, nullptr, &_layout));

	VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pSetLayouts = &_layout;
	allocInfo.descriptorSetCount = 1;
	allocInfo.descriptorPool = descPool;

	VK_CHECK(vkAllocateDescriptorSets(_deviceObject.GetDevice(), &allocInfo, &_set));
}

VulkanDescriptor::~VulkanDescriptor()
{
	VkDevice device = _deviceObject.GetDevice();
	VkDescriptorSetLayout layout = _layout;
	VulkanDeleter::SubmitObjectDesctruction([device, layout]()
		{
			vkDestroyDescriptorSetLayout(device, layout, nullptr);
		});
}

void VulkanDescriptor::Write(u32 dstBinding, u32 dstArrayElem, DescriptorType type, Image* image, Sampler* sampler)
{
	assert(image && sampler && "Can't write to descriptor set, image or sampler is null");


	VulkanImage* rawImage     = static_cast<VulkanImage*>(image);
	VulkanSampler* rawSampler = static_cast<VulkanSampler*>(sampler);

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageView = rawImage->GetRawView();
	switch (type)
	{
	case DescriptorType::COMBINED_IMAGE_SAMPLER:
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		break;
	
	case DescriptorType::STORAGE_IMAGE:
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		break;

	default: std::unreachable();
	}

	imgInfo.sampler = rawSampler->GetRawSampler();

	VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeSet.dstSet = _set;
	writeSet.dstBinding = dstBinding;
	writeSet.dstArrayElement = dstArrayElem;
	writeSet.descriptorCount = 1;
	writeSet.descriptorType = ToVkDescriptorType(type);
	writeSet.pImageInfo = &imgInfo;


	vkUpdateDescriptorSets(_deviceObject.GetDevice(), 1, &writeSet, 0, nullptr);
}

void VulkanDescriptor::Write(u32 dstBinding, u32 dstArrayElem, RTAccelerationStructure* accel, Image* image)
{
	VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeSet.dstSet = _set;
	writeSet.dstBinding = dstBinding;
	writeSet.dstArrayElement = dstArrayElem;
	writeSet.descriptorCount = 1;

	if (image)
	{
		VulkanImage* rawImage = static_cast<VulkanImage*>(image);

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageView = rawImage->GetRawView();
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // the best layout for storage image


		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writeSet.pImageInfo = &imgInfo;

		vkUpdateDescriptorSets(_deviceObject.GetDevice(), 1, &writeSet, 0, nullptr);
	}
	else if(accel)
	{
		VulkanAccelerationStructure* rawAccel = static_cast<VulkanAccelerationStructure*>(accel);
		VkAccelerationStructureKHR vkAccel = rawAccel->GetRaw();

		VkWriteDescriptorSetAccelerationStructureKHR descAs{
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
		descAs.accelerationStructureCount = 1;
		descAs.pAccelerationStructures = &vkAccel;

		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		writeSet.pNext = &descAs;

		vkUpdateDescriptorSets(_deviceObject.GetDevice(), 1, &writeSet, 0, nullptr);
	}
	else
		assert(false && "Trying to update ray tracing desc. set but image and accel structure both null");
}

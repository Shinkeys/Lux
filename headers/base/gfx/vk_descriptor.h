#pragma once
#include "../../util/gfx/vk_types.h"
#include "vk_device.h"
#include "../core/descriptor.h"

inline VkDescriptorType ToVkDescriptorType(DescriptorType type)
{
    switch (type)
    {
    case DescriptorType::COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	case DescriptorType::SAMPLED_IMAGE:			 return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case DescriptorType::STORAGE_IMAGE:			 return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	case DescriptorType::ACCELERATION_STRUCTURE: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    default:
        std::unreachable();
    }
}




class Image;
class Sampler;
class RTAccelerationStructure;
// DON'T CREATE IT MANUALLY, IT SHOULD BE CREATED VIA DESCRIPTOR BASE CLASS
class VulkanDescriptor : public Descriptor
{
private:
	VulkanDevice& _deviceObject;

	VkDescriptorSet		  _set{ VK_NULL_HANDLE };
	VkDescriptorSetLayout _layout{ VK_NULL_HANDLE };

public:
	VulkanDescriptor(const DescriptorSpecification& spec, VulkanDevice& deviceObj, VkDescriptorPool descPool);
	~VulkanDescriptor();

	// OBJECT MANAGED VIA SHARED PTR
	VulkanDescriptor(const VulkanDescriptor&) = delete;
	VulkanDescriptor& operator=(const VulkanDescriptor&) = delete;
	VulkanDescriptor(VulkanDescriptor&&) noexcept = delete;
	VulkanDescriptor& operator=(VulkanDescriptor&&) noexcept = delete;

	VkDescriptorSet GetRawSet()				const { return _set; }
	VkDescriptorSetLayout GetRawSetLayout() const { return _layout; }

	
	void Write(u32 dstBinding, u32 dstArrayElem, DescriptorType type, Image* image, Sampler* sampler) override;
	void Write(u32 dstBinding, u32 dstArrayElem, RTAccelerationStructure* accel = nullptr, Image* image = nullptr) override;
};


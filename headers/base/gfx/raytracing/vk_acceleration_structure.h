#pragma once
#include "../../../util/gfx/vk_types.h"
#include "../../core/raytracing/RT_acceleration_structure.h"


class VulkanDevice;
class VulkanFrame;
class VulkanAllocator;
class Buffer;

class VulkanAccelerationStructure : public RTAccelerationStructure
{
private:
	VulkanDevice& _deviceObj;


	VkAccelerationStructureKHR _acceleration{ VK_NULL_HANDLE };

	std::unique_ptr<Buffer> _asBuffer{ nullptr };	

	
public:

	VulkanAccelerationStructure(VulkanDevice& deviceObj, VulkanFrame& frameObj, 
		VulkanAllocator& allocatorObj,  const BLASSpecification& blasSpec);

	VulkanAccelerationStructure(VulkanDevice& deviceObj, VulkanFrame& frameObj,
		VulkanAllocator& allocatorObj, const TLASSpecification& tlasSpec);

	~VulkanAccelerationStructure();

	u64 GetAccelerationAddress() const override;

	VkAccelerationStructureKHR GetRaw() const { return _acceleration; }

};
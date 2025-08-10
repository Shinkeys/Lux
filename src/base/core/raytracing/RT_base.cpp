#include "../../../headers/base/core/raytracing/RT_base.h"
#include "../../../headers/base/gfx/vk_base.h"
#include "../../../headers/base/gfx/raytracing/vk_acceleration_structure.h"

RayTracingManager::RayTracingManager(VulkanBase& vulkanBase) : _vulkanBase{vulkanBase}
{

}


std::unique_ptr<RTAccelerationStructure>	 RayTracingManager::CreateBLAS(const BLASSpecification& spec)	 const
{
	return std::make_unique<VulkanAccelerationStructure>(_vulkanBase.GetVulkanDeviceObj(), _vulkanBase.GetFrameObj(), _vulkanBase.GetAllocatorObj(), spec);
}


std::unique_ptr<RTAccelerationStructure>	 RayTracingManager::CreateTLAS(const TLASSpecification& spec)	 const
{
	return std::make_unique<VulkanAccelerationStructure>(_vulkanBase.GetVulkanDeviceObj(), _vulkanBase.GetFrameObj(), _vulkanBase.GetAllocatorObj(), spec);
}

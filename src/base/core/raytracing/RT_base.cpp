#include "../../../headers/base/core/raytracing/RT_base.h"
#include "../../../headers/base/gfx/vk_base.h"
#include "../../../headers/base/gfx/raytracing/vk_rt_pipeline.h"
#include "../../../headers/base/gfx/raytracing/vk_acceleration_structure.h"
#include "../../../headers/base/core/raytracing/shader_binding_table.h"
#include "../../../headers/base/core/raytracing/RT_pipeline.h"
#include "../../../headers/util/rt_types.h"
#include "../../../headers/base/core/buffer.h"

RayTracingManager::RayTracingManager(VulkanBase& vulkanBase, BufferManager& buffManager) : _vulkanBase{vulkanBase}, _bufferManager{buffManager}
{

}


RTDeviceProperties RayTracingManager::GetRTDeviceProperties() const
{
	return _vulkanBase.GetVulkanDeviceObj().GetRTDeviceProps();
}


void RayTracingManager::GetRTGroupHandles(RTPipeline* rtPipeline, u32 firstGroup, u32 groupCount, u32 dataSize, void* outData) const
{
	VulkanRTPipeline* rawPipeline = static_cast<VulkanRTPipeline*>(rtPipeline);

	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(_vulkanBase.GetVulkanDeviceObj().GetDevice(), rawPipeline->GetRawPipeline(), firstGroup, groupCount, dataSize, outData));
}

std::unique_ptr<RTAccelerationStructure>	 RayTracingManager::CreateBLAS(const BLASSpecification& spec)	 const
{
	return std::make_unique<VulkanAccelerationStructure>(_vulkanBase.GetVulkanDeviceObj(), _vulkanBase.GetFrameObj(), _vulkanBase.GetAllocatorObj(), spec);
}


std::unique_ptr<RTAccelerationStructure>	 RayTracingManager::CreateTLAS(const TLASSpecification& spec)	 const
{
	return std::make_unique<VulkanAccelerationStructure>(_vulkanBase.GetVulkanDeviceObj(), _vulkanBase.GetFrameObj(), _vulkanBase.GetAllocatorObj(), spec);
}


std::unique_ptr<ShaderBindingTable>         RayTracingManager::CreateSBT(const  SBTSpecification& spec)      const
{
	return std::make_unique<ShaderBindingTable>(_vulkanBase.GetVulkanDeviceObj().GetRTDeviceProps(), spec, _bufferManager);
}

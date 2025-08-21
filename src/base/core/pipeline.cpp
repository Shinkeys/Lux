#include "../../../headers/base/core/pipeline.h"
#include "../../../headers/base/core/raytracing/RT_pipeline.h"
#include "../../../headers/base/gfx/raytracing/vk_rt_pipeline.h"
#include "../../../headers/base/gfx/vk_base.h"


PipelineManager::PipelineManager(VulkanBase& vulkanBase) : _vulkanBase{vulkanBase}
{

}

std::unique_ptr<Pipeline> PipelineManager::CreatePipeline(const PipelineSpecification& spec)
{
	return std::make_unique<VulkanPipeline>(spec, _vulkanBase.GetVulkanDeviceObj(), _vulkanBase.GetShaderObj());
}

std::unique_ptr<RTPipeline> PipelineManager::CreateRTPipeline(const RTPipelineSpecification& spec)
{
	return std::make_unique<VulkanRTPipeline>(spec, _vulkanBase.GetVulkanDeviceObj(), _vulkanBase.GetShaderObj());
}
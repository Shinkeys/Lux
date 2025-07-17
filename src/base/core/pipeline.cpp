#include "../../../headers/base/core/pipeline.h"
#include "../../../headers/base/gfx/vk_base.h"


PipelineManager::PipelineManager(VulkanBase& vulkanBase) : _vulkanBase{vulkanBase}
{

}

std::unique_ptr<Pipeline> PipelineManager::CreatePipeline(const PipelineSpecification& spec)
{
	return std::make_unique<VulkanPipeline>(spec, _vulkanBase.GetVulkanDeviceObj(), _vulkanBase.GetShaderObj());
}
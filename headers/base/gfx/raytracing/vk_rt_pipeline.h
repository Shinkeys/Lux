#pragma once
#include "../../../util/gfx/vk_types.h"
#include "../../core/raytracing/RT_pipeline.h"

class VulkanShader;
class VulkanDevice;
class VulkanRTPipeline : public RTPipeline
{
private:
	VulkanDevice& _deviceObject;
	VulkanShader& _shaderObject;

	VkPipeline _pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout _layout{ VK_NULL_HANDLE };

	RTPipelineSpecification _specification;
public:
	VulkanRTPipeline(const RTPipelineSpecification& spec, VulkanDevice& deviceObj, VulkanShader& shaderObj);

	~VulkanRTPipeline();

	VkPipeline GetRawPipeline()     const { return _pipeline; }
	VkPipelineLayout GetRawLayout() const { return _layout; }
};

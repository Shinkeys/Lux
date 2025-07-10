#pragma once
#include "vk_presentation.h"
#include "../core/pipeline.h"

namespace vkconversions
{
	VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology topology);
	VkPolygonMode ToVkPolygonMode(PolygonMode mode);
	VkCullModeFlagBits ToVkCullMode(CullMode mode);
	VkFrontFace ToVkFrontFace(FrontFace face);
	VkCompareOp ToVkCompareOP(CompareOP op);
	VkAccessFlags2 ToVkAccessFlags2(AccessFlag flags);
	VkPipelineStageFlags2 ToVkPipelineStageFlags2(PipelineStage stages);
}

// DON'T CREATE IT MANUALLY, IT SHOULD BE CREATED VIA PIPELINE BASE CLASS
class VulkanPipeline : public Pipeline
{
private:
	VulkanDevice& _deviceObject;
	VkPipeline _pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout _layout{ VK_NULL_HANDLE };

	PipelineSpecification _specification;

	void CreateGraphicsPipeline();
	void CreateComputePipeline();
public:
	const PipelineSpecification& GetSpecification() override { return _specification; }

	VulkanPipeline(const PipelineSpecification& spec, VulkanDevice& deviceObj);
	
	VulkanPipeline() = delete;
	~VulkanPipeline() = default;

	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline(VulkanPipeline&&) = delete;
	VulkanPipeline& operator= (const VulkanPipeline&) = delete;
	VulkanPipeline& operator= (VulkanPipeline&&) = delete;



	VkPipeline GetRawPipeline()     const { return _pipeline; }
	VkPipelineLayout GetRawLayout() const { return _layout;   }

	void Cleanup();
};
#pragma once
#include "vk_presentation.h"

class VulkanPipeline
{
private:
	VulkanDevice& _deviceObject;
	VulkanPresentation& _presentationObject;
	void CreatePipeline();

	// To do. for uniforms
	VkPipelineLayout _pipelineLayout;
	VkRenderPass _renderPass;
	VkPipeline _graphicsPipeline;
public:
	VkRenderPass GetVkRenderPass()		   const { return _renderPass; }
	VkPipeline GetVkPipeline()		       const { return _graphicsPipeline; }
	VkPipelineLayout GetVkPipelineLayout() const { return _pipelineLayout; }

	void SetDynamicStates(VkCommandBuffer cmdBuffer) const;
	VulkanPipeline() = delete;
	~VulkanPipeline() = default;
	VulkanPipeline(VulkanDevice& deviceObj, VulkanPresentation& presentationObj);


	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline(VulkanPipeline&&) = delete;
	VulkanPipeline& operator= (const VulkanPipeline&) = delete;
	VulkanPipeline& operator= (VulkanPipeline&&) = delete;

	void Cleanup();
};
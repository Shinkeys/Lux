#pragma once
#include "vk_presentation.h"


// Type shader name without extensions ---
// Incorrect: shading.vert
// Correct: shading
struct GraphicsPipeline
{
	std::filesystem::path shaderName{};
	VkPrimitiveTopology topology{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
	VkPolygonMode polygonMode{ VK_POLYGON_MODE_FILL };
	VkCullModeFlagBits cullMode{ VK_CULL_MODE_NONE };
	VkFrontFace frontFace{ VK_FRONT_FACE_COUNTER_CLOCKWISE }; // CCW because flipped viewport
	VkBool32 depthTestEnable{ VK_TRUE };
	VkBool32 depthWriteEnable{ VK_TRUE };
	VkCompareOp depthCompare{ VK_COMPARE_OP_GREATER };
	VkFormat colorFormat{ VulkanPresentation::ColorFormat.format };

	std::vector<VkDescriptorSetLayout> descriptorLayouts;

	u32 pushConstantSizeBytes{ 0 };
	u32 pushConstantOffset{ 0 };

	bool operator==(const GraphicsPipeline& other) const
	{
		return shaderName == other.shaderName && colorFormat == other.colorFormat &&
				cullMode == other.cullMode;
	}
};

template <> 
struct std::hash<GraphicsPipeline>
{
	size_t operator()(const GraphicsPipeline& pipeline) const noexcept
	{
		size_t h1 = std::hash<fs::path>{}(pipeline.shaderName); // since C++ 20
		size_t h2 = std::hash<VkFormat>{}(pipeline.colorFormat);
		size_t h3 = std::hash<VkCullModeFlagBits>{}(pipeline.cullMode); 

		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};

struct PipelinePair
{
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
};


class VulkanPipeline
{
private:
	VulkanDevice& _deviceObject;
	VulkanPresentation& _presentationObject;

	// To do. for uniforms
	std::unordered_map<GraphicsPipeline, PipelinePair> _pipelinesCache;
public:
	PipelinePair CreatePipeline(const GraphicsPipeline& graphicsPipeline);
	void SetDynamicStates(VkCommandBuffer cmdBuffer) const;
	const PipelinePair* GetPipelinePair(const GraphicsPipeline& graphicsPipeline) const;

	VulkanPipeline() = delete;
	~VulkanPipeline() = default;
	VulkanPipeline(VulkanDevice& deviceObj, VulkanPresentation& presentationObj);


	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline(VulkanPipeline&&) = delete;
	VulkanPipeline& operator= (const VulkanPipeline&) = delete;
	VulkanPipeline& operator= (VulkanPipeline&&) = delete;

	void Cleanup();
};
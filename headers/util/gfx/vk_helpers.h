#pragma once
#include "vk_types.h"
#include "../logger.h"
#include "../helpers.h"


namespace vkhelpers
{
	// Purpose: load shader from file and create VkShaderModule
	// Output: program crash if file couldn't be open.
	// Pass only shaders' name. Example: shading.vert
	std::optional<VkShaderModule> ReadShaderFile(const fs::path& shaderPath, VkDevice device);
	VkCommandBufferBeginInfo CmdBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);


	/**
* @brief Transitions an image layout in a Vulkan command buffer.
* @param cmd The command buffer to record the barrier into.
* @param image The Vulkan image to transition.
* @param oldLayout The current layout of the image.
* @param newLayout The desired new layout of the image.
* @param srcAccessMask The source access mask, specifying which access types are being transitioned from.
* @param dstAccessMask The destination access mask, specifying which access types are being transitioned to.
* @param srcStage The pipeline stage that must happen before the transition.
* @param dstStage The pipeline stage that must happen after the transition.
*/
	void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout,
		VkAccessFlags2 srcAccessMask, VkAccessFlags2 dstAccessMask, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask);

	VkImageCreateInfo CreateImageInfo(VkFormat imgFormat, VkExtent3D imgExtent, u32 mipLevels, VkImageUsageFlags usageFlags);
}
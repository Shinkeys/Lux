#include "../../../headers/base/gfx/vk_renderer.h"
#include "../../../headers/util/gfx/vk_helpers.h"

VulkanRenderer::VulkanRenderer(VulkanBase& vulkanBase) : _vulkanBase{ vulkanBase }
{

}


void VulkanRenderer::BeginFrame()
{
	VulkanFrame& frameObject = _vulkanBase.GetFrameObj();
	frameObject.BeginFrame();
	frameObject.BeginCommandRecord();
}

void VulkanRenderer::EndFrame()
{
	VulkanFrame& frameObject = _vulkanBase.GetFrameObj();
	frameObject.EndCommandRecord();
	ExecuteCurrentCommands();
	frameObject.EndFrame();
}

void VulkanRenderer::ExecuteCurrentCommands()
{
	const auto graphicsQueue = _vulkanBase.GetVulkanDeviceObj().GetQueueByType(QueueType::VULKAN_GRAPHICS_QUEUE);

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

	VkPipelineStageFlags waitStages[]
	{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	VkSemaphore waitSemaphores[] = { _vulkanBase.GetFrameObj().GetImageAvailableSemaphore()};
	VkSemaphore signalSemaphores[] = { _vulkanBase.GetFrameObj().GetRenderFinishedSemaphore()};

	VkCommandBuffer cmdBuff = _vulkanBase.GetFrameObj().GetCommandBuffer();

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuff;

	// signal when cmd buffers finished exec
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK(vkQueueSubmit(graphicsQueue.value(), 1, &submitInfo, _vulkanBase.GetFrameObj().GetFence()));
}

void VulkanRenderer::BeginRender(const std::vector<std::shared_ptr<ImageHandle>>& attachments, glm::vec4 clearColor)
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();
	const VulkanSwapchain& swapchainDesc = _vulkanBase.GetPresentationObj().GetSwapchainDesc();
	const u32 imageIndex = _vulkanBase.GetFrameObj().GetCurrentImageIndex();

	std::vector<VkRenderingAttachmentInfo> colorAttachments;
	std::vector<VkRenderingAttachmentInfo> depthAttachments;
	for (auto attachment : attachments)
	{

		if (attachment->usage == ImageUsage::USAGE_RENDER_TARGET)
		{
			// Dynamic rendering requires layout transitions. access mask is the first layer of synchronization while stage is the second layer.
			// looks like the first one is what you need to protect in the memory and the second one when to finish this, in this case fragment shader output
			VkImageLayout newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			vkhelpers::TransitionImageLayout(cmdBuffer, attachment, attachment->currentLayout, newLayout,
				VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);


			VkClearValue clearColor{ {0.0f, 0.0f, 0.0f, 1.0f} };

			VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			colorAttachment.imageView = attachment->imageView;
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue = clearColor;


			colorAttachments.push_back(colorAttachment);
		}
		else if (attachment->usage == ImageUsage::USAGE_DEPTH_TARGET)
		{
			vkhelpers::TransitionImageLayout(cmdBuffer, attachment, attachment->currentLayout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

			VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			depthAttachment.imageView = attachment->imageView;
			depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

			if (clearColor.w != 0.0f)
				depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			else
				depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;


			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.clearValue = { .depthStencil{1.0f, 0} };

			depthAttachments.push_back(depthAttachment);
		}
	}

	// Begin rendering
	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.renderArea =
	{
		.offset = {0,0},
		.extent = swapchainDesc.extent
	};
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = static_cast<u32>(colorAttachments.size());
	renderingInfo.pColorAttachments = colorAttachments.data();
	renderingInfo.pDepthAttachment = depthAttachments.empty() ? nullptr : depthAttachments.data();

	_vulkanBase.GetPipelineObj().SetDynamicStates(cmdBuffer);
	vkCmdBeginRendering(cmdBuffer, &renderingInfo);
}

void VulkanRenderer::RenderMesh(const DrawCommand& drawCommand)
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	const bool areObjectsInitialized = drawCommand.pipeline != VK_NULL_HANDLE && drawCommand.pipelineLayout != VK_NULL_HANDLE && drawCommand.descriptorSet != VK_NULL_HANDLE &&
		drawCommand.indexBuffer != VK_NULL_HANDLE;

	assert(areObjectsInitialized && "Vulkan draw command is dispatched with some NULL object");

	// To DO: compute tasks when would need
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawCommand.pipeline);
	vkCmdBindIndexBuffer(cmdBuffer, drawCommand.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawCommand.pipelineLayout, 0, 1, &drawCommand.descriptorSet, 0, nullptr);
	vkCmdPushConstants(cmdBuffer, drawCommand.pipelineLayout, VK_SHADER_STAGE_ALL, 0, drawCommand.pushConstants.size, drawCommand.pushConstants.data);
	vkCmdDrawIndexed(cmdBuffer, drawCommand.indexCount, 1, 0, 0, 0);


	delete[] drawCommand.pushConstants.data;
}

// WOULD FLUSH THIS STRUCTURE
void VulkanRenderer::ExecuteBarriers(PipelineBarrierStorage& barriers)
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	for (const auto& barrier : barriers.memoryBarriers)
	{
		vkhelpers::InsertMemoryBarrier(cmdBuffer, barrier.srcStageMask, barrier.dstStageMask, barrier.srcAccessMask, barrier.dstAccessMask);
	}

	for (auto& barrier : barriers.imageBarriers)
	{
		vkhelpers::TransitionImageLayout(cmdBuffer, barrier.imgHandle, barrier.imgHandle->currentLayout, barrier.newLayout, barrier.srcAccessMask,
			barrier.dstAccessMask, barrier.srcStageMask, barrier.dstStageMask, VK_IMAGE_ASPECT_COLOR_BIT);

	}

	barriers.imageBarriers.clear();
	barriers.memoryBarriers.clear();

}


void VulkanRenderer::DispatchCompute(const DispatchCommand& dispatchCommand)
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, dispatchCommand.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, dispatchCommand.pipelineLayout, 0, 1, &dispatchCommand.descriptorSet, 0, nullptr);
	vkCmdPushConstants(cmdBuffer, dispatchCommand.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, dispatchCommand.pushConstants.size, dispatchCommand.pushConstants.data);
	vkCmdDispatch(cmdBuffer, dispatchCommand.numWorkgroups.x, dispatchCommand.numWorkgroups.y, dispatchCommand.numWorkgroups.z);

	delete[] dispatchCommand.pushConstants.data;
}

void VulkanRenderer::EndRender()
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	vkCmdEndRendering(cmdBuffer);
}

void VulkanRenderer::RenderQuad(const DrawCommand& drawCommand)
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	vkCmdPushConstants(cmdBuffer, drawCommand.pipelineLayout, VK_SHADER_STAGE_ALL, 0, drawCommand.pushConstants.size, drawCommand.pushConstants.data);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawCommand.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawCommand.pipelineLayout, 0, 1, &drawCommand.descriptorSet, 0, nullptr);

	vkCmdDraw(cmdBuffer, 6, 1, 0, 0);

	delete[] drawCommand.pushConstants.data;
}


//void BeginFrame(u32 imageIndex) override;
//void EndFrame(u32 imageIndex) override;
//void BeginRender() override; // to do;
//void WaitForFence() override;
//void ResetFence() override;
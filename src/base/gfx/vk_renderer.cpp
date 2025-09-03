#include "../../../headers/base/gfx/vk_renderer.h"
#include "../../../headers/util/gfx/vk_helpers.h"
#include "../../../headers/base/core/pipeline_types.h"
#include "../../../headers/base/core/raytracing/shader_binding_table.h"
#include "../../../headers/base/core/raytracing/RT_pipeline.h"
#include "../../../headers/base/gfx/raytracing/vk_rt_pipeline.h"

VulkanRenderer::VulkanRenderer(VulkanBase& vulkanBase) : _vulkanBase{ vulkanBase }
{

}


void VulkanRenderer::BeginFrame() const
{
	VulkanFrame& frameObject = _vulkanBase.GetFrameObj();
	frameObject.BeginFrame();
	frameObject.BeginCommandRecord();
}

void VulkanRenderer::EndFrame() const
{
	VulkanFrame& frameObject = _vulkanBase.GetFrameObj();
	frameObject.EndCommandRecord();
	ExecuteCurrentCommands();
	frameObject.EndFrame();
}

u32 VulkanRenderer::GetCurrentImageIndex() const
{
	return _vulkanBase.GetFrameObj().GetCurrentImageIndex();
}

u32 VulkanRenderer::GetCurrentFrameIndex() const
{
	return _vulkanBase.GetFrameObj().GetCurrentFrameIndex();
}

void VulkanRenderer::ExecuteCurrentCommands() const
{
	const auto graphicsQueue = _vulkanBase.GetVulkanDeviceObj().GetQueueByType(QueueType::VULKAN_GENERAL_QUEUE);

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

void VulkanRenderer::BeginRender(const std::vector<Image*>& attachments, glm::vec4 clearColor) const
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();
	const VulkanSwapchain& swapchainDesc = _vulkanBase.GetPresentationObj().GetSwapchainDesc();
	const u32 imageIndex = _vulkanBase.GetFrameObj().GetCurrentImageIndex();

	std::vector<VkRenderingAttachmentInfo> colorAttachments;
	std::vector<VkRenderingAttachmentInfo> depthAttachments;
	for (auto attachment : attachments)
	{

		if (attachment->GetSpecification().usage & ImageUsage::IMAGE_USAGE_COLOR_ATTACHMENT)
		{
			VulkanImage* rawImage = static_cast<VulkanImage*>(attachment);

			assert(rawImage && "Raw vulkan image is nullptr in the BeginRender(), color attachment part");

			// Dynamic rendering requires layout transitions. access mask is the first layer of synchronization while stage is the second layer.
			// looks like the first one is what you need to protect in the memory and the second one when to finish this, in this case fragment shader output
			vkhelpers::TransitionImageLayout(cmdBuffer, rawImage->GetRawImage(),
				vkconversions::ToVkImageLayout(attachment->GetSpecification().layout),
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				vkconversions::ToVkAspectFlags(attachment->GetSpecification().aspect));

			rawImage->SetCurrentLayout(ImageLayout::IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);


			VkClearValue vkClearColor{ clearColor.x, clearColor.y, clearColor.z, clearColor.w };


			VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			colorAttachment.imageView = rawImage->GetRawView();
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.clearValue = vkClearColor;


			colorAttachments.push_back(colorAttachment);
		}
		else if (attachment->GetSpecification().usage & ImageUsage::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT)
		{
			VulkanImage* rawImage = static_cast<VulkanImage*>(attachment);

			assert(rawImage && "Raw vulkan image is nullptr in the BeginRender(), depth stencil attachment part");

			VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			depthAttachment.imageView = rawImage->GetRawView();
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

	VkExtent2D swapchainExtent = _vulkanBase.GetPresentationObj().GetSwapchainDesc().extent;
	// Dynamic states
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = static_cast<float>(swapchainExtent.height);
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = -static_cast<float>(swapchainExtent.height); // Flipping viewport to change Y positive direction
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);


	vkCmdBeginRendering(cmdBuffer, &renderingInfo);
}

void VulkanRenderer::RenderMesh(const DrawCommand& drawCommand) const
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	const bool areObjectsInitialized = drawCommand.pipeline && drawCommand.descriptor &&
		drawCommand.indexBuffer != VK_NULL_HANDLE;

	assert(areObjectsInitialized && "Vulkan draw command is dispatched with some NULL object");


	VulkanPipeline* rawPipeline     = static_cast<VulkanPipeline*>(drawCommand.pipeline);
	VulkanDescriptor* rawDescriptor = static_cast<VulkanDescriptor*>(drawCommand.descriptor);

	assert(rawPipeline && rawDescriptor && "After trying to cast from base to derived object VulkanPipeline or VulkanDescriptor is null in RenderMesh()");

	VkDescriptorSet descriptorSet = rawDescriptor->GetRawSet();


	// To DO: compute tasks when would need
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rawPipeline->GetRawPipeline());
	vkCmdBindIndexBuffer(cmdBuffer, drawCommand.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rawPipeline->GetRawLayout(), 0, 1, &descriptorSet, 0, nullptr);
	if(drawCommand.pushConstants.data)
		vkCmdPushConstants(cmdBuffer, rawPipeline->GetRawLayout(), VK_SHADER_STAGE_ALL, 0, drawCommand.pushConstants.size, drawCommand.pushConstants.data);
	vkCmdDrawIndexed(cmdBuffer, drawCommand.indexCount, 1, 0, 0, 0);

	if (drawCommand.pushConstants.data)
		delete[] drawCommand.pushConstants.data;
}

// WOULD FLUSH THIS STRUCTURE
void VulkanRenderer::ExecuteBarriers(PipelineBarrierStorage& barriers) const
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	std::vector<VkImageMemoryBarrier2> imageBarriers;
	std::vector<VkMemoryBarrier2> memoryBarriers;
	for (const auto& barrierSpecification : barriers.memoryBarriers)
	{
		VkAccessFlags srcAccess = vkconversions::ToVkAccessFlags2(barrierSpecification.srcAccessMask);
		VkAccessFlags dstAccess = vkconversions::ToVkAccessFlags2(barrierSpecification.dstAccessMask);

		VkPipelineStageFlags2 srcStage = vkconversions::ToVkPipelineStageFlags2(barrierSpecification.srcStageMask);
		VkPipelineStageFlags2 dstStage = vkconversions::ToVkPipelineStageFlags2(barrierSpecification.dstStageMask);


		VkMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
		barrier.srcStageMask = srcStage;
		barrier.dstStageMask = dstStage;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;

		memoryBarriers.push_back(barrier);
	}

	for (auto& barrierSpecification : barriers.imageBarriers)
	{
		if (barrierSpecification.image == nullptr)
			continue;

		VkAccessFlags srcAccess = vkconversions::ToVkAccessFlags2(barrierSpecification.srcAccessMask);
		VkAccessFlags dstAccess = vkconversions::ToVkAccessFlags2(barrierSpecification.dstAccessMask);

		VkPipelineStageFlags2 srcStage = vkconversions::ToVkPipelineStageFlags2(barrierSpecification.srcStageMask);
		VkPipelineStageFlags2 dstStage = vkconversions::ToVkPipelineStageFlags2(barrierSpecification.dstStageMask);

		VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		barrier.oldLayout = vkconversions::ToVkImageLayout(barrierSpecification.image->GetSpecification().layout);
		barrier.newLayout = vkconversions::ToVkImageLayout(barrierSpecification.newLayout);
		barrier.srcAccessMask = vkconversions::ToVkAccessFlags2(barrierSpecification.srcAccessMask);
		barrier.dstAccessMask = vkconversions::ToVkAccessFlags2(barrierSpecification.dstAccessMask);
		barrier.srcStageMask  = vkconversions::ToVkPipelineStageFlags2(barrierSpecification.srcStageMask);
		barrier.dstStageMask  = vkconversions::ToVkPipelineStageFlags2(barrierSpecification.dstStageMask);

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;


		VulkanImage* rawImage = static_cast<VulkanImage*>(barrierSpecification.image);

		barrier.image = rawImage->GetRawImage();

		barrier.subresourceRange =
		{
			.aspectMask = vkconversions::ToVkAspectFlags(barrierSpecification.aspect),
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		rawImage->SetCurrentLayout(barrierSpecification.newLayout);

		imageBarriers.push_back(barrier);
	}
	VkDependencyInfo dependencyInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.dependencyFlags = 0;

	dependencyInfo.imageMemoryBarrierCount = static_cast<u32>(imageBarriers.size());
	dependencyInfo.pImageMemoryBarriers = imageBarriers.empty() ? nullptr : imageBarriers.data();

	dependencyInfo.memoryBarrierCount = static_cast<u32>(memoryBarriers.size());
	dependencyInfo.pMemoryBarriers = memoryBarriers.empty() ? nullptr : memoryBarriers.data();

	vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);


	barriers.imageBarriers.clear();
	barriers.memoryBarriers.clear();
}


void VulkanRenderer::DispatchCompute(const DispatchCommand& dispatchCommand) const
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	VulkanPipeline* rawPipeline     = static_cast<VulkanPipeline*>(dispatchCommand.pipeline);
	VulkanDescriptor* rawDescriptor = static_cast<VulkanDescriptor*>(dispatchCommand.descriptor);

	assert(rawPipeline && rawDescriptor && "After trying to cast from base to derived object VulkanPipeline or VulkanDescriptor is null in DispatchCompute()");


	VkDescriptorSet descriptorSet = rawDescriptor->GetRawSet();

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, rawPipeline->GetRawPipeline());
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, rawPipeline->GetRawLayout(), 0, 1, &descriptorSet, 0, nullptr);
	if(dispatchCommand.pushConstants.data)
		vkCmdPushConstants(cmdBuffer, rawPipeline->GetRawLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, dispatchCommand.pushConstants.size, dispatchCommand.pushConstants.data);
	vkCmdDispatch(cmdBuffer, dispatchCommand.numWorkgroups.x, dispatchCommand.numWorkgroups.y, dispatchCommand.numWorkgroups.z);

	if (dispatchCommand.pushConstants.data)
		delete[] dispatchCommand.pushConstants.data;
}

void VulkanRenderer::EndRender() const
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	vkCmdEndRendering(cmdBuffer);
}

void VulkanRenderer::RenderQuad(const DrawCommand& drawCommand) const 
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	VulkanPipeline* rawPipeline        = static_cast<VulkanPipeline*>(drawCommand.pipeline);
	VulkanDescriptor* rawDescriptorSet = static_cast<VulkanDescriptor*>(drawCommand.descriptor);

	assert(rawPipeline && rawDescriptorSet && "After trying to cast from base to derived object VulkanPipeline or VulkanDescriptor is null in RenderQuad()");

	VkDescriptorSet descriptor = rawDescriptorSet->GetRawSet();

	vkCmdPushConstants(cmdBuffer, rawPipeline->GetRawLayout(), VK_SHADER_STAGE_ALL, 0, drawCommand.pushConstants.size, drawCommand.pushConstants.data);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rawPipeline->GetRawPipeline());
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rawPipeline->GetRawLayout(), 0, 1, &descriptor, 0, nullptr);

	vkCmdDraw(cmdBuffer, 6, 1, 0, 0);

	if (drawCommand.pushConstants.data)
	{
		delete[] drawCommand.pushConstants.data;
	}
}

void VulkanRenderer::RenderIndirect(const RenderIndirectCountCommand& command) const
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	VulkanPipeline* rawPipeline = static_cast<VulkanPipeline*>(command.pipeline);
	VulkanDescriptor* rawDescriptorSet = static_cast<VulkanDescriptor*>(command.descriptor);

	assert(rawPipeline && rawDescriptorSet && command.indexBuffer && 
		"After trying to cast from base to derived object VulkanPipeline or VulkanDescriptor is null in RenderQuad()");


	VulkanBuffer* rawIndirectBuffer = static_cast<VulkanBuffer*>(command.buffer);

	VulkanBuffer* rawIndexBuffer = static_cast<VulkanBuffer*>(command.indexBuffer);

	if (command.pushConstants.data)
	{
		vkCmdPushConstants(cmdBuffer, rawPipeline->GetRawLayout(), VK_SHADER_STAGE_ALL, 0, 
			command.pushConstants.size, command.pushConstants.data);
	}

	VkDescriptorSet descriptor = rawDescriptorSet->GetRawSet();

	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rawPipeline->GetRawLayout(), 0, 1, &descriptor, 0, nullptr);
	vkCmdBindIndexBuffer(cmdBuffer, rawIndexBuffer->GetRawBuffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rawPipeline->GetRawPipeline());
	vkCmdDrawIndexedIndirectCount(cmdBuffer, rawIndirectBuffer->GetRawBuffer(), 0, 
		rawIndirectBuffer->GetRawBuffer(), 
		command.countBufferOffsetBytes, // count buffer is the same buffer, but in the end
		command.maxDrawCount, sizeof(VkDrawIndexedIndirectCommand));

	if (command.pushConstants.data)
	{
		delete[] command.pushConstants.data;
	}
}


void VulkanRenderer::RenderRayTracing(const RTDrawCommand& drawCommand) const
{
	VkCommandBuffer cmdBuffer = _vulkanBase.GetFrameObj().GetCommandBuffer();

	VulkanRTPipeline* rawPipeline = static_cast<VulkanRTPipeline*>(drawCommand.rtPipeline);
	VulkanDescriptor* rawDescriptorSet = static_cast<VulkanDescriptor*>(drawCommand.descriptor);
	
	assert(rawPipeline && rawDescriptorSet &&
		"After trying to cast from base to derived object VulkanPipeline or VulkanDescriptor is null in RenderRayTracing()");
	

	VkDescriptorSet descriptor = rawDescriptorSet->GetRawSet();
	
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rawPipeline->GetRawPipeline());
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rawPipeline->GetRawLayout(), 0, 1, &descriptor, 0, nullptr);
	
	if (drawCommand.pushConstants.data)
	{
		vkCmdPushConstants(cmdBuffer, rawPipeline->GetRawLayout(), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0,
			drawCommand.pushConstants.size, drawCommand.pushConstants.data);
	}

	SBTRegion raygenTable = drawCommand.sbt->GetRaygenTable();
	SBTRegion missTable = drawCommand.sbt->GetMissTable();
	SBTRegion closestTable = drawCommand.sbt->GetClosestTable();

	VkStridedDeviceAddressRegionKHR raygenSBT = { raygenTable.address, raygenTable.stride, raygenTable.size };
	VkStridedDeviceAddressRegionKHR missSBT = { missTable.address, missTable.stride, missTable.size };
	VkStridedDeviceAddressRegionKHR hitSBT = { closestTable.address, closestTable.stride, closestTable.size };
	VkStridedDeviceAddressRegionKHR callableSBT = {}; // Empty(don't use call shaders as for now)

	VkExtent2D screenExt = _vulkanBase.GetPresentationObj().GetSwapchainDesc().extent;

	vkCmdTraceRaysKHR(cmdBuffer, &raygenSBT, &missSBT, &hitSBT, &callableSBT, screenExt.width, screenExt.height, 1);


	if (drawCommand.pushConstants.data)
	{
		delete[] drawCommand.pushConstants.data;
	}
}



//void BeginFrame(u32 imageIndex) override;
//void EndFrame(u32 imageIndex) override;
//void BeginRender() override; // to do;
//void WaitForFence() override;
//void ResetFence() override;
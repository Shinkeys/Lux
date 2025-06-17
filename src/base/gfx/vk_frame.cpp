#include "../../../headers/base/gfx/vk_frame.h"
#include "../../../headers/util/gfx/vk_helpers.h"

VulkanFrame::VulkanFrame(VulkanDevice& deviceObj, VulkanPresentation& presentationObj, VulkanPipeline& pipelineObj) : 
				  _deviceObject{ deviceObj }, _presentationObject {presentationObj}, _pipelineObject{ pipelineObj }
{
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSynchronizationObjects();
}

VkSemaphore VulkanFrame::GetImageAvailableSemaphore() const
{
	return _imageAvailableSemaphores[_currentFrame];
}
// This one should be the same as the swapchain image count
VkSemaphore VulkanFrame::GetRenderFinishedSemaphore(u32 imageIndex)  const
{
	return _renderFinishedSemaphores[imageIndex];
}
VkFence VulkanFrame::GetFence() const
{
	return _syncCPUFences[_currentFrame];
}
VkCommandBuffer VulkanFrame::GetCommandBuffer() const
{
	return _commandBuffers[_currentFrame];
}

void VulkanFrame::CreateCommandPool()
{
	VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	auto queueFamilyIndex = _deviceObject.GetQueueIndexByType(QueueType::VULKAN_GRAPHICS_QUEUE);
	Logger::Log("Cannot create command pool, there are no graphics queue family", queueFamilyIndex.has_value(), LogLevel::Fatal);

	createInfo.queueFamilyIndex = queueFamilyIndex.value();

	VK_CHECK(vkCreateCommandPool(_deviceObject.GetDevice(), &createInfo, nullptr, &_commandPool));
	Logger::Log("Created graphics command pool", _commandPool, LogLevel::Debug);
}

void VulkanFrame::CreateCommandBuffers()
{
	_commandBuffers.resize(FramesInFlight);

	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = _commandPool;
	allocateInfo.commandBufferCount = static_cast<u32>(_commandBuffers.size());
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VK_CHECK(vkAllocateCommandBuffers(_deviceObject.GetDevice(), &allocateInfo, _commandBuffers.data()));

	for(i32 i = 0; i < FramesInFlight; ++i)
		Logger::Log("Allocated graphics command buffer", _commandBuffers[i], LogLevel::Debug);
}

void VulkanFrame::CreateSynchronizationObjects()
{
	const VkDevice device = _deviceObject.GetDevice();
	
	VkSemaphoreCreateInfo semCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


	_imageAvailableSemaphores.resize(FramesInFlight);
	_syncCPUFences.resize(FramesInFlight);

	const size_t imagesInSwapchain = _presentationObject.GetSwapchainDesc().images.size();
	_renderFinishedSemaphores.resize(imagesInSwapchain);

	for (size_t i = 0; i < FramesInFlight; ++i)
	{
		VK_CHECK(vkCreateSemaphore(device, &semCreateInfo, nullptr, &_imageAvailableSemaphores[i]));
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &_syncCPUFences[i]));
	}
	for (size_t i = 0; i < imagesInSwapchain; ++i)
	{
		VK_CHECK(vkCreateSemaphore(device, &semCreateInfo, nullptr, &_renderFinishedSemaphores[i]));
	}
}

void VulkanFrame::RecordCommandBuffer(u32 imageIndex)
{
	VkCommandBuffer cmdBuffer = _commandBuffers[_currentFrame];
	const VulkanSwapchain& swapchainDesc = _presentationObject.GetSwapchainDesc();

	vkResetCommandBuffer(cmdBuffer, 0);

	VkCommandBufferBeginInfo beginInfo = vkhelpers::CmdBufferBeginInfo();
	
	VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
	
	// Dynamic rendering requires layout transitions. access mask is the first layer of synchronization while stage is the second layer.
	// looks like the first one is what you need to protect in the memory and the second one when to finish this, in this case fragment shader output
	vkhelpers::TransitionImageLayout(cmdBuffer, swapchainDesc.images[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

	VkClearValue clearColor{ {0.0f, 0.0f, 0.0f, 1.0f} };

	VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	colorAttachment.imageView = swapchainDesc.imagesView[imageIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue = clearColor;

	// Begin rendering
	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.renderArea =
	{
		.offset = {0,0},
		.extent = swapchainDesc.extent
	};
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;

	vkCmdBeginRendering(cmdBuffer, &renderingInfo);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineObject.GetVkPipeline());

	_pipelineObject.SetDynamicStates(cmdBuffer);

	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

	vkCmdEndRendering(cmdBuffer);


	vkhelpers::TransitionImageLayout(cmdBuffer, swapchainDesc.images[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0, 
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer));
}


void VulkanFrame::WaitForFence()
{
	vkWaitForFences(_deviceObject.GetDevice(), 1, &_syncCPUFences[_currentFrame], VK_TRUE, UINT64_MAX);
}

void VulkanFrame::ResetFence()
{
	vkResetFences(_deviceObject.GetDevice(), 1, &_syncCPUFences[_currentFrame]);
}

void VulkanFrame::Cleanup()
{
	const VkDevice device = _deviceObject.GetDevice();

	vkDestroyCommandPool(device, _commandPool, nullptr);
	for (i32 i = 0; i < FramesInFlight; ++i)
	{
		vkDestroySemaphore(device, _imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, _syncCPUFences[i], nullptr);
	}
	for (VkSemaphore semaphore : _renderFinishedSemaphores)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
	}
}
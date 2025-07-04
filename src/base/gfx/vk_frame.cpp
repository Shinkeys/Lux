#include "../../../headers/base/gfx/vk_frame.h"
#include "../../../headers/base/gfx/vk_image.h"
#include "../../../headers/util/gfx/vk_helpers.h"
#include "../../../headers/base/core/renderer.h"

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
VkSemaphore VulkanFrame::GetRenderFinishedSemaphore()  const
{
	return _renderFinishedSemaphores[_currentImage];
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

void VulkanFrame::BeginFrame()
{
	VkDevice device = _deviceObject.GetDevice();
	const VulkanSwapchain& swapchainDesc = _presentationObject.GetSwapchainDesc();
	// Image index - swapchain image index

	WaitForFence();
	VkResult swapchainResultImageStage = vkAcquireNextImageKHR(device, swapchainDesc.swapchain,
		UINT64_MAX, GetImageAvailableSemaphore(), VK_NULL_HANDLE, &_currentImage);

	if (swapchainResultImageStage == VK_ERROR_OUT_OF_DATE_KHR || swapchainResultImageStage == VK_SUBOPTIMAL_KHR)
	{
		_presentationObject.RecreateSwapchain();
		return; // Go and get a new frame
	}

	ResetFence();

	VkSemaphore semaphoreRenderFinished = GetRenderFinishedSemaphore();
}

void VulkanFrame::EndFrame()
{
	VkSemaphore waitSemaphores[] =   { GetImageAvailableSemaphore() };
	VkSemaphore signalSemaphores[] = { GetRenderFinishedSemaphore() };
	const auto graphicsQueue = _deviceObject.GetQueueByType(QueueType::VULKAN_GRAPHICS_QUEUE);
	const auto presentationQueue = _deviceObject.GetQueueByType(QueueType::VULKAN_PRESENTATION_QUEUE);


	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	std::vector<VkSwapchainKHR> swapchains
	{ _presentationObject.GetSwapchainDesc().swapchain };
	presentInfo.swapchainCount = static_cast<u32>(swapchains.size());
	presentInfo.pSwapchains = swapchains.data();
	presentInfo.pImageIndices = &_currentImage;

	if (!presentationQueue.has_value())
	{
		std::cout << "Critical error, presentation queue is nullptr";
		return;
	}

	VkResult swapchainResultPresentStage = vkQueuePresentKHR(presentationQueue.value(), &presentInfo);
	if (swapchainResultPresentStage == VK_ERROR_OUT_OF_DATE_KHR || swapchainResultPresentStage == VK_SUBOPTIMAL_KHR)
	{
		_presentationObject.RecreateSwapchain();
	}

	UpdateCurrentFrameIndex();
}

void VulkanFrame::BeginCommandRecord()
{
	VkCommandBuffer cmdBuffer = _commandBuffers[_currentFrame];
	vkResetCommandBuffer(cmdBuffer, 0);

	VkCommandBufferBeginInfo beginInfo = vkhelpers::CmdBufferBeginInfo();

	VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
}

void VulkanFrame::EndCommandRecord()
{
	VkCommandBuffer cmdBuffer = _commandBuffers[_currentFrame];
	const VulkanSwapchain& swapchainDesc = _presentationObject.GetSwapchainDesc();

	assert(!swapchainDesc.images.empty() && "Vulkan swapchain images is empty in EndCommandRecord()");

	vkhelpers::TransitionImageLayout(cmdBuffer, swapchainDesc.images[_currentImage]->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer));
}

void VulkanFrame::WaitForFence()
{
	VK_CHECK(vkWaitForFences(_deviceObject.GetDevice(), 1, &_syncCPUFences[_currentFrame], VK_TRUE, UINT64_MAX));
}

void VulkanFrame::ResetFence()
{
	VK_CHECK(vkResetFences(_deviceObject.GetDevice(), 1, &_syncCPUFences[_currentFrame]));
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
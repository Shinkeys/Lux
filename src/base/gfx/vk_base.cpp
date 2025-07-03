#include "../../../headers/base/gfx/vk_base.h"
#include "../../../headers/base/core/renderer.h"

// DEFINE FOR VOLK SHOULD BE PLACED ONLY HERE
#define VOLK_IMPLEMENTATION
#include <volk.h>


void VulkanBase::Initialize(Window& windowObj)
{
	_instanceObject = std::make_unique<VulkanInstance>(windowObj);
	_deviceObject = std::make_unique<VulkanDevice>(*_instanceObject);
	_presentationObject = std::make_unique<VulkanPresentation>(*_instanceObject, *_deviceObject, windowObj);
	_pipelineObject = std::make_unique<VulkanPipeline>(*_deviceObject, *_presentationObject);
	_frameObject = std::make_unique<VulkanFrame>(*_deviceObject, *_presentationObject, *_pipelineObject);
	_descriptorObject = std::make_unique<VulkanDescriptor>(*_deviceObject);
	_allocatorObject = std::make_unique<VulkanAllocator>(*_instanceObject, *_deviceObject);
	_bufferObject = std::make_unique<VulkanBuffer>(*_instanceObject, *_deviceObject, *_allocatorObject);
	_imageObject = std::make_unique<VulkanImage>(*_deviceObject, *_bufferObject, *_frameObject, *_allocatorObject);
}

void VulkanBase::RenderFrame()
{
	const VkDevice device = _deviceObject->GetDevice();
	const VulkanSwapchain& swapchainDesc = _presentationObject->GetSwapchainDesc();
	const auto graphicsQueue = _deviceObject->GetQueueByType(QueueType::VULKAN_GRAPHICS_QUEUE);
	const auto presentationQueue = _deviceObject->GetQueueByType(QueueType::VULKAN_PRESENTATION_QUEUE);
	const VkCommandBuffer cmdBuffer = _frameObject->GetCommandBuffer();
	VkSemaphore semaphoreImageAvailable = _frameObject->GetImageAvailableSemaphore();
	VkFence syncCPUFence = _frameObject->GetFence();

	if (!graphicsQueue.has_value())
	{
		std::cout << "Critical error, graphics queue is nullptr";
		return;
	}

	// Image index - swapchain image index
	u32 imageIndex;

	_frameObject->WaitForFence();
	VkResult swapchainResultImageStage = vkAcquireNextImageKHR(device, swapchainDesc.swapchain, 
										 UINT64_MAX, semaphoreImageAvailable, VK_NULL_HANDLE, &imageIndex);

	if (swapchainResultImageStage == VK_ERROR_OUT_OF_DATE_KHR || swapchainResultImageStage == VK_SUBOPTIMAL_KHR)
	{
		_presentationObject->RecreateSwapchain();
		return; // Go and get a new frame
	}

	_frameObject->ResetFence();

	VkSemaphore semaphoreRenderFinished = _frameObject->GetRenderFinishedSemaphore(imageIndex);

	_frameObject->BeginFrame(imageIndex);
	_imageObject->UpdateLayoutsToCopyData();
	_descriptorObject->UpdateSets();
	// Render all the objects connected to vulkan
	// It contains smth like: bind pipeline, vkCmdDraw etc	
	_frameObject->BeginRendering(imageIndex);
	Renderer::RenderScene();
	_frameObject->EndRendering(imageIndex);

	_frameObject->EndFrame(imageIndex);

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	
	VkPipelineStageFlags waitStages[]
	{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	VkSemaphore waitSemaphores[] = { semaphoreImageAvailable };
	VkSemaphore signalSemaphores[] = { semaphoreRenderFinished };


	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores  = waitSemaphores;
	submitInfo.pWaitDstStageMask  = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	// signal when cmd buffers finished exec
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK(vkQueueSubmit(graphicsQueue.value(), 1, &submitInfo, syncCPUFence));

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	std::vector<VkSwapchainKHR> swapchains
	{ swapchainDesc.swapchain };
	presentInfo.swapchainCount = static_cast<u32>(swapchains.size());
	presentInfo.pSwapchains = swapchains.data();
	presentInfo.pImageIndices = &imageIndex;

	if (!presentationQueue.has_value())
	{
		std::cout << "Critical error, presentation queue is nullptr";
		return;
	}

	VkResult swapchainResultPresentStage = vkQueuePresentKHR(presentationQueue.value(), &presentInfo);
	if (swapchainResultPresentStage == VK_ERROR_OUT_OF_DATE_KHR || swapchainResultPresentStage == VK_SUBOPTIMAL_KHR)
	{
		_presentationObject->RecreateSwapchain();
	}

	_frameObject->UpdateCurrentFrameIndex();
}


void VulkanBase::Cleanup()
{
	vkDeviceWaitIdle(_deviceObject->GetDevice());
	// Instance should be destroyed last
	_descriptorObject->Cleanup();
	_imageObject->Cleanup();
	_bufferObject->Cleanup();
	_allocatorObject->Cleanup();
	_frameObject->Cleanup();
	_pipelineObject->Cleanup();
	_presentationObject->Cleanup();
	_deviceObject->Cleanup();
	_instanceObject->Cleanup();
}
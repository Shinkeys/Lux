#pragma once
#include "vk_pipeline.h"



struct VulkanDrawCommand
{
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDeviceAddress objectBufferAddress{ 0 };
	VkDeviceAddress uniformBufferAddress{ 0 };
	VkBuffer indexBuffer{ VK_NULL_HANDLE };
	u32 indexCount{ 0 };
};

class VulkanFrame
{
private:
	VulkanDevice& _deviceObject;
	VulkanPresentation& _presentationObject;
	VulkanPipeline& _pipelineObject;

	//std::vector<VkFramebuffer> _framebuffers;

	VkCommandPool   _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;

	std::vector<VkSemaphore> _imageAvailableSemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _syncCPUFences;

	// Variables
	static constexpr i32 FramesInFlight{ 2 };
	i32 _currentFrame{ 0 };

	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronizationObjects();
public:
	VulkanFrame() = delete;
	~VulkanFrame() = default;
	VulkanFrame(VulkanDevice& deviceObj, VulkanPresentation& presentationObj, VulkanPipeline& pipelineObj);


	VulkanFrame(const VulkanFrame&) = delete;
	VulkanFrame(VulkanFrame&&) = delete;
	VulkanFrame& operator= (const VulkanFrame&) = delete;
	VulkanFrame& operator= (VulkanFrame&&) = delete;

	void BeginFrame(u32 imageIndex);
	void EndFrame(u32 imageIndex);
	void WaitForFence();
	void ResetFence();
	void Cleanup();
	void SubmitRenderTask(const VulkanDrawCommand& drawCommand) const;
	void UpdateCurrentFrameIndex() { _currentFrame = (_currentFrame + 1) % FramesInFlight; }
	i32 GetCurrentFrameIndex()							   const { return _currentFrame; }

	template<typename T>
	void SubmitUniformBuffer(const T& buffer) const;

	VkSemaphore GetImageAvailableSemaphore()			   const;
	VkSemaphore GetRenderFinishedSemaphore(u32 imageIndex) const;
	VkFence GetFence()                                     const;
	VkCommandBuffer GetCommandBuffer()                     const;
};


template<typename T>
void VulkanFrame::SubmitUniformBuffer(const T& buffer) const
{

}
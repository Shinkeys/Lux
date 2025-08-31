#pragma once
#include "vk_pipeline.h"

struct Attachments
{
	std::vector<VkImage> images;
	std::vector<VkImageView> imagesView;
};

class VulkanImage;
class VulkanFrame
{
private:
	VulkanDevice& _deviceObject;
	VulkanPresentation& _presentationObject;

	//std::vector<VkFramebuffer> _framebuffers;

	VkCommandPool   _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;

	std::vector<VkSemaphore> _imageAvailableSemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _syncCPUFences;
	

	u32 _currentFrame{ 0 };
	u32 _currentImage{ 0 };

	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronizationObjects();
public:
	VulkanFrame() = delete;
	~VulkanFrame() = default;
	VulkanFrame(VulkanDevice& deviceObj, VulkanPresentation& presentationObj);


	VulkanFrame(const VulkanFrame&) = delete;
	VulkanFrame(VulkanFrame&&) = delete;
	VulkanFrame& operator= (const VulkanFrame&) = delete;
	VulkanFrame& operator= (VulkanFrame&&) = delete;

	// Variables
	static constexpr u32 FramesInFlight{ 2 };
	void BeginFrame();
	void EndFrame();
	void BeginCommandRecord();
	void EndCommandRecord();
	void WaitForFence();
	void ResetFence();

	void SubmitMainCommandBuffer();

	void UpdateCurrentFrameIndex() { _currentFrame = (_currentFrame + 1) % FramesInFlight; }
	u32 GetCurrentFrameIndex()							   const { return _currentFrame; }
	u32 GetCurrentImageIndex()							   const { return _currentImage; }

	VkSemaphore GetImageAvailableSemaphore()			   const;
	VkSemaphore GetRenderFinishedSemaphore()		       const;
	VkFence GetFence()                                     const;
	VkCommandBuffer GetCommandBuffer()                     const;
	VkCommandPool GetCommandPool()                         const { return _commandPool; }

	void Cleanup();
};


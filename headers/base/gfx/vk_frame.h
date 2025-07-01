#pragma once
#include "vk_pipeline.h"

enum class RenderJobType : u8
{
	GEOMETRY_PASS,
};

// Vulkan draw COMMAND entity based. all those resources should be binded on ENTITY basis. Other resources independent of entity, but
// dependent of scene should be bound with VulkanBindCommonResources
struct VulkanDrawCommand
{
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	std::vector<VkDeviceAddress> buffersAddresses;
	VkBuffer indexBuffer{ VK_NULL_HANDLE };
	u32 indexCount{ 0 };

	RenderJobType type{ RenderJobType::GEOMETRY_PASS };
};


struct VulkanBindCommonResources
{
	std::vector<VkDeviceAddress> buffersAddresses;
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

	// TO DO SMTH ELSE IF WOULD NEED LIKE TEXTURES, UNIFORMS ETC. 

	RenderJobType type{ RenderJobType::GEOMETRY_PASS };
};

class VulkanImage;
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

	// Variables
	static constexpr i32 FramesInFlight{ 2 };
	void BeginFrame(u32 imageIndex);
	void EndFrame(u32 imageIndex);
	void BeginRendering(u32 imageIndex);
	void EndRendering(u32 imageIndex);
	void WaitForFence();
	void ResetFence();
	void Cleanup();
	void SubmitRenderTask(const VulkanDrawCommand& drawCommand) const;
	void SubmitBindingOfCommonResources(const VulkanBindCommonResources& resources) const;

	void UpdateCurrentFrameIndex() { _currentFrame = (_currentFrame + 1) % FramesInFlight; }
	i32 GetCurrentFrameIndex()							   const { return _currentFrame; }

	VkSemaphore GetImageAvailableSemaphore()			   const;
	VkSemaphore GetRenderFinishedSemaphore(u32 imageIndex) const;
	VkFence GetFence()                                     const;
	VkCommandBuffer GetCommandBuffer()                     const;
};


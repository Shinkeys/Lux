#pragma once
#include "../gfx/vk_base.h"

// Renderer now is vulkan only. If would need different API, then can create base class renderer API
// Purpose: every render job should be submitted to this class. It's implemented on top of lambda function
// You write lambda function with all the needed setup in your class and then  submit it to this class which would
// handle the rendering.
class Renderer
{
private:
	using RenderJob = std::function<void()>;

	static std::deque<RenderJob> _renderJobs;
	static VulkanBase* _vulkanBackend;
public:
	static u32 GetCurrentFrameIndex();
	static void RenderScene();
	static void Submit(RenderJob&& renderJob);


	static void Initialize(VulkanBase& vulkanBase);
};
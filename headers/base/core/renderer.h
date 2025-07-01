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
	using BindingJob = std::function<void()>;

	static std::vector<std::pair<RenderJobType, RenderJob>> _renderJobs;
	static std::vector<std::pair<RenderJobType, BindingJob>> _bindingJobs;
	static VulkanBase* _vulkanBackend;
public:
	static u32 GetCurrentFrameIndex();
	static void RenderScene();
	static void SubmitDrawJob(RenderJob&& renderJob, RenderJobType jobType);
	static void SubmitBindingJob(BindingJob&& bindingJob, RenderJobType renderStage);

	static void Initialize(VulkanBase& vulkanBase);
};
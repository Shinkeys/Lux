#include "../../../headers/base/core/renderer.h"

VulkanBase* Renderer::_vulkanBackend{ nullptr };
std::deque<Renderer::RenderJob> Renderer::_renderJobs;


void Renderer::Initialize(VulkanBase& vulkanBase)
{
	_vulkanBackend = &vulkanBase;
}

u32 Renderer::GetCurrentFrameIndex()
{
	return _vulkanBackend->GetFrameObj().GetCurrentFrameIndex();
}
	
void Renderer::Submit(RenderJob&& renderJob)
{
	_renderJobs.emplace_back(renderJob);
}

// Draw all the data passed in
void Renderer::RenderScene()
{
	for (auto it = _renderJobs.begin(); it != _renderJobs.end(); ++it)
	{
		(*it)();
	}

	_renderJobs.clear();
}
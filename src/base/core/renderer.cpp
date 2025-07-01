#include "../../../headers/base/core/renderer.h"

VulkanBase* Renderer::_vulkanBackend{ nullptr };
std::vector<std::pair<RenderJobType, Renderer::RenderJob>> Renderer::_renderJobs;
std::vector<std::pair<RenderJobType, Renderer::BindingJob>> Renderer::_bindingJobs;


void Renderer::Initialize(VulkanBase& vulkanBase)
{
	_vulkanBackend = &vulkanBase;
}

u32 Renderer::GetCurrentFrameIndex()
{
	return _vulkanBackend->GetFrameObj().GetCurrentFrameIndex();
}
	
void Renderer::SubmitDrawJob(RenderJob&& renderJob, RenderJobType jobType)
{
	_renderJobs.emplace_back(jobType, renderJob);
}

void Renderer::SubmitBindingJob(BindingJob&& bindingJob, RenderJobType renderStage)
{
	_bindingJobs.emplace_back(renderStage, bindingJob);
}

// Draw all the data passed in
void Renderer::RenderScene()
{
	// Sort em by priority
	std::sort(_renderJobs.begin(), _renderJobs.end(), [&](const std::pair<RenderJobType, RenderJob>& fstJob, const std::pair<RenderJobType, RenderJob>& scJob)
		{
			return fstJob.first < scJob.first;
		});

	std::sort(_bindingJobs.begin(), _bindingJobs.end(), [&](const std::pair<RenderJobType, BindingJob>& fstJob, const std::pair<RenderJobType, BindingJob>& scJob)
		{
			return fstJob.first < scJob.first;
		});

	for (auto renderJobIt = _renderJobs.begin(); renderJobIt != _renderJobs.end(); ++renderJobIt)
	{
		for (const auto& bindJob : _bindingJobs)
		{
			if (bindJob.first == renderJobIt->first)
				(bindJob.second)(); // Bind all the structures with the same type
			else break; // we need to rebind every structure again for every draw call.
		}

		(renderJobIt->second)();
	}

	_renderJobs.clear();
	_bindingJobs.clear();
}
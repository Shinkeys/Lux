#pragma once
#include "../../util/gfx/vk_types.h"
#include "vk_base.h"
#include "../core/rendererAPI.h"


class VulkanRenderer : public RendererAPI
{
private:
	VulkanBase& _vulkanBase;

	void BeginFrame()																const override;
	void EndFrame()																	const override;
	void BeginRender(const std::vector<Image*>& attachments, glm::vec4 clearColor)  const override; // to do;
	void EndRender()																const override;

	void RenderMesh(const DrawCommand& command)										const override;
	void RenderQuad(const DrawCommand& drawCommand)									const override;
	void RenderIndirect(const RenderIndirectCountCommand& command)					const override;

	void ExecuteCurrentCommands()													const override;
	void ExecuteBarriers(PipelineBarrierStorage& barriers)							const override;
	void DispatchCompute(const DispatchCommand& dispatchCommand)					const override;

	void RenderRayTracing(const RTDrawCommand& drawCommand)							const override;

	u32 GetCurrentImageIndex()														const override;
	u32 GetCurrentFrameIndex()														const override;

public:

	VulkanRenderer(VulkanBase& vulkanBase);

};
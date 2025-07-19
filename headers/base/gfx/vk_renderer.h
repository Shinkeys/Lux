#pragma once
#include "../../util/gfx/vk_types.h"
#include "vk_base.h"
#include "../core/rendererAPI.h"


class VulkanRenderer : public RendererAPI
{
private:
	VulkanBase& _vulkanBase;

	void BeginFrame() override;
	void EndFrame() override;
	void BeginRender(const std::vector<Image*>& attachments, glm::vec4 clearColor) override; // to do;
	void EndRender() override;

	void RenderMesh(const DrawCommand& command) override;
	void RenderQuad(const DrawCommand& drawCommand) override;
	void RenderIndirect(const RenderIndirectCountCommand& command) override;

	void ExecuteCurrentCommands() override;
	void ExecuteBarriers(PipelineBarrierStorage& barriers) override;
	void DispatchCompute(const DispatchCommand& dispatchCommand) override;

public:

	VulkanRenderer(VulkanBase& vulkanBase);

};
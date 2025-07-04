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
	void BeginRender(const std::vector<std::shared_ptr<ImageHandle>>& attachments) override; // to do;
	void EndRender() override;

	void RenderMesh(const DrawCommand& command);


	void ExecuteCurrentCommands() override;

public:

	VulkanRenderer(VulkanBase& vulkanBase);

};
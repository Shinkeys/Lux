#pragma once
#include "../../util/util.h"

// Maybe someday to do next level of abstraction to hide this objects from the classes. Not needed right now for sure.
struct DrawCommand
{
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	LightPushConsts lightPushConstants{};
	VkBuffer indexBuffer{ VK_NULL_HANDLE };
	u32 indexCount{ 0 };
};


// Basic class from which renderer would inherit.
class RendererAPI
{
private:

public:
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void BeginRender(const std::vector<std::shared_ptr<ImageHandle>>& attachments) = 0; // to do;
	virtual void EndRender() = 0;
	virtual void ExecuteCurrentCommands() = 0;
	virtual void RenderMesh(const DrawCommand& command) = 0;

	virtual ~RendererAPI() = default;


};

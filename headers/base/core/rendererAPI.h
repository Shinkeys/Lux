#pragma once
#include "../../util/util.h"


// Due to Push Constants system it's a must to use something like this structure to map
// all the data CONSEQUENTIAL
struct PushConsts
{
	byte* data;
	u32 size;
};

// Maybe someday to do next level of abstraction to hide this objects from the classes. Not needed right now for sure.
struct DrawCommand
{
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	PushConsts pushConstants{};
	VkBuffer indexBuffer{ VK_NULL_HANDLE };
	u32 indexCount{ 0 };
};

struct DispatchCommand
{
	glm::ivec3 numWorkgroups{ 0 };

	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	PushConsts pushConstants{};
};

// Basic class from which renderer would inherit.
class RendererAPI
{
private:

public:
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void BeginRender(const std::vector<std::shared_ptr<ImageHandle>>& attachments, glm::vec4 clearColor) = 0; // to do;
	virtual void EndRender() = 0;
	virtual void ExecuteCurrentCommands() = 0;
	virtual void RenderMesh(const DrawCommand& drawCommand) = 0;
	virtual void RenderQuad(const DrawCommand& drawCommand) = 0;
	virtual void ExecuteBarriers(PipelineBarrierStorage& barriers) = 0;
	virtual void DispatchCompute(const DispatchCommand& dispatchCommand) = 0;

	virtual ~RendererAPI() = default;


};

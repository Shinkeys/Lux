#pragma once
#include "../gfx/vk_renderer.h"
#include "rendererAPI.h"


// Renderer now is vulkan only. If would need different API, then can create base class renderer API
// Purpose: every render job should be submitted to this class. It's implemented on top of lambda function
// You write lambda function with all the needed setup in your class and then  submit it to this class which would
// handle the rendering.
class Renderer
{
private:
	static RendererAPI* _renderAPI;
public:
	static void ExecuteRecordedCommands();


	// ENGINE NOW WORKS ONLY WITH VULKAN. IF WOULD NEED TO CHANGE IT, JUST CREATE A STRUCT WITH DIFFERENT TYPES OF RENDERERS
	static void Initialize(VulkanBase& vulkanBase);

	static void BeginFrame();
	static void EndFrame();
	static void BeginRender(const std::vector<Image*>& attachments, glm::vec4 clearColor);
	static void EndRender();
	static void RenderMesh(const DrawCommand& command);
	static void RenderIndirect(const RenderIndirectCountCommand& command);
	static void RenderQuad(const DrawCommand& drawCommand);
	// WOULD FLUSH THIS STRUCTURE
	static void ExecuteBarriers(PipelineBarrierStorage& barriers);
	static void DispatchCompute(const DispatchCommand& dispatchCommand);

	static void RenderRayTracing(const RTDrawCommand& drawCommand);


	static u32 GetCurrentImageIndex();
	static u32 GetCurrentFrameIndex();


	static void Cleanup();
};
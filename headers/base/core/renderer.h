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
	static void BeginRender(const std::vector<std::shared_ptr<ImageHandle>>& attachments);
	static void EndRender();
	static void RenderMesh(const DrawCommand& command);


	static void Cleanup();
};
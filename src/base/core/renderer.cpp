#include "../../../headers/base/core/renderer.h"

RendererAPI* Renderer::_renderAPI{ nullptr };

void Renderer::Initialize(VulkanBase& vulkanBase)
{
	_renderAPI = new VulkanRenderer(vulkanBase);
}

void Renderer::BeginFrame()
{
	_renderAPI->BeginFrame();
}

void Renderer::EndFrame()
{
	_renderAPI->EndFrame();
}

void Renderer::BeginRender(const std::vector<std::shared_ptr<ImageHandle>>& attachments, glm::vec4 clearColor)
{
	_renderAPI->BeginRender(attachments, clearColor);
}

void Renderer::EndRender()
{
	_renderAPI->EndRender();
}

// Draw all the data passed in
void Renderer::ExecuteRecordedCommands()
{
	_renderAPI->ExecuteCurrentCommands();
}

void Renderer::RenderMesh(const DrawCommand& command)
{
	_renderAPI->RenderMesh(command);
}

void Renderer::ExecuteBarriers(PipelineBarrierStorage& barriers)
{
	_renderAPI->ExecuteBarriers(barriers);
}

void Renderer::DispatchCompute(const DispatchCommand& dispatchCommand)
{
	_renderAPI->DispatchCompute(dispatchCommand);
}

void Renderer::RenderQuad(const DrawCommand& drawCommand)
{
	_renderAPI->RenderQuad(drawCommand);
}


void Renderer::Cleanup()
{
	delete _renderAPI;
}
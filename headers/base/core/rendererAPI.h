#pragma once
#include "../../util/util.h"


// Due to Push Constants system it's a must to use something like this structure to map
// all the data CONSEQUENTIAL
struct PushConsts
{
	byte* data{ nullptr };
	u32 size{ 0 };
};

struct RenderIndirectCountCommand
{
	Pipeline* pipeline{ nullptr };
	Descriptor* descriptor{ nullptr };
	PushConsts pushConstants{};
	Buffer* buffer{ nullptr };
	u32 maxDrawCount{ 1 };

	Buffer* indexBuffer{ nullptr };

	u32 countBufferOffsetBytes{ 0 };
};

struct IndirectPushConst
{
	u64 vertexAddress{ 0 };
	u64 commonMeshDataAddress{ 0 };
	u64 commonMeshIndicesAddress{ 0 };
	u64 viewDataAddress{ 0 };

	u32 baseDrawOffset{ 0 };
};

struct DrawCommand
{
	Pipeline* pipeline{ nullptr };
	Descriptor* descriptor{ nullptr };
	PushConsts pushConstants{};
	// TO ABSTRACT BUFFERS!!!!!
	VkBuffer indexBuffer{ VK_NULL_HANDLE };
	u32 indexCount{ 0 };
};


struct DispatchCommand
{
	Pipeline* pipeline{ nullptr };
	Descriptor* descriptor{ nullptr };
	PushConsts pushConstants{};
	glm::ivec3 numWorkgroups{ 0 };
};


// Basic class from which renderer would inherit.
class RendererAPI
{
private:

public:
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void BeginRender(const std::vector<Image*>& attachments, glm::vec4 clearColor) = 0; // to do;
	virtual void EndRender() = 0;
	virtual void ExecuteCurrentCommands() = 0;
	virtual void RenderMesh(const DrawCommand& drawCommand) = 0;
	virtual void RenderQuad(const DrawCommand& drawCommand) = 0;
	virtual void RenderIndirect(const RenderIndirectCountCommand& command) = 0;
	virtual void ExecuteBarriers(PipelineBarrierStorage& barriers) = 0;
	virtual void DispatchCompute(const DispatchCommand& dispatchCommand) = 0;

	virtual ~RendererAPI() = default;


};

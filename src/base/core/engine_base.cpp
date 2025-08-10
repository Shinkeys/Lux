#include "../../../headers/base/core/engine_base.h"
#include "../../../headers/base/core/image.h"
#include "../../../headers/base/core/descriptor.h"
#include "../../../headers/base/core/presentation_manager.h"
#include "../../../headers/base/core/frame_manager.h"
#include "../../../headers/base/core/raytracing/RT_base.h"



EngineBase::EngineBase(VulkanBase& vulkanBase) : _vulkanBase{ vulkanBase }
{
	_imageManager        = std::make_unique<ImageManager>(_vulkanBase);
	_descriptorManager   = std::make_unique<DescriptorManager>(_vulkanBase);
	_pipelineManager     = std::make_unique<PipelineManager>(_vulkanBase);
	_bufferManager       = std::make_unique<BufferManager>(_vulkanBase);
	_frameManager        = std::make_unique<FrameManager>(_vulkanBase);
	_presentationManager = std::make_unique<PresentationManager>(_vulkanBase);
	_rayTracingManager   = std::make_unique<RayTracingManager>(_vulkanBase);
}

EngineBase::~EngineBase()
{
	_descriptorManager->Cleanup();
}


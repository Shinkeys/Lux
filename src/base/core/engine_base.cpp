#include "../../../headers/base/core/engine_base.h"
#include "../../../headers/base/core/image.h"
#include "../../../headers/base/core/descriptor.h"



EngineBase::EngineBase(VulkanBase& vulkanBase) : _vulkanBase{ vulkanBase }
{
	_imageManager = std::make_unique<ImageManager>(_vulkanBase);
	_descriptorManager = std::make_unique<DescriptorManager>(_vulkanBase);
	_pipelineManager = std::make_unique<PipelineManager>(_vulkanBase);
}

EngineBase::~EngineBase()
{
	_descriptorManager->Cleanup();
}


#pragma once
#include "../gfx/vk_base.h"


class ImageManager;
class DescriptorManager;
class PipelineManager;
// Purpose: main class to abstract all the API logic behind.
class EngineBase
{
private:
	VulkanBase& _vulkanBase;

	std::unique_ptr<ImageManager>	   _imageManager;
	std::unique_ptr<DescriptorManager> _descriptorManager;
	std::unique_ptr<PipelineManager>   _pipelineManager;
public:
	ImageManager& GetImageManager()			  { return *_imageManager; }
	DescriptorManager& GetDescriptorManager() { return *_descriptorManager; }
	PipelineManager&   GetPipelineManager() { return   *_pipelineManager; }

	EngineBase() = delete;
	~EngineBase() = default;
	EngineBase(VulkanBase& vulkanBase);


	EngineBase(const EngineBase&) = delete;
	EngineBase(EngineBase&&) = delete;
	EngineBase& operator= (const EngineBase&) = delete;
	EngineBase& operator= (EngineBase&&) = delete;
};
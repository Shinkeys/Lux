#pragma once
#include "../gfx/vk_base.h"


class ImageManager;
class DescriptorManager;
class PipelineManager;
class BufferManager;
// Purpose: main class to abstract all the API logic behind.
class EngineBase
{
private:
	VulkanBase& _vulkanBase;

	std::unique_ptr<ImageManager>	   _imageManager;
	std::unique_ptr<DescriptorManager> _descriptorManager;
	std::unique_ptr<PipelineManager>   _pipelineManager;
	std::unique_ptr<BufferManager>     _bufferManager;
public:
	ImageManager& GetImageManager()			  { return *_imageManager; }
	DescriptorManager& GetDescriptorManager() { return *_descriptorManager; }
	PipelineManager&   GetPipelineManager() { return   *_pipelineManager; }
	BufferManager&   GetBufferManager() { return   *_bufferManager; }

	EngineBase() = delete;
	~EngineBase();
	EngineBase(VulkanBase& vulkanBase);


	EngineBase(const EngineBase&) = delete;
	EngineBase(EngineBase&&) = delete;
	EngineBase& operator= (const EngineBase&) = delete;
	EngineBase& operator= (EngineBase&&) = delete;
};
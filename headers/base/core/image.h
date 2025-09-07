#pragma once
#include "image_types.h"
#include "pipeline_types.h"


struct ImageSpecification
{
	std::filesystem::path path{};

	ImageType type{ ImageType::IMAGE_TYPE_NONE };
	ImageFormat format{ ImageFormat::IMAGE_FORMAT_R8G8B8A8_SRGB };
	ImageExtent3D extent{ 0, 0, 0 };
	ImageUsage usage{ ImageUsage::IMAGE_USAGE_COLOR_ATTACHMENT };
	ImageLayout layout{ ImageLayout::IMAGE_LAYOUT_UNDEFINED };
	ImageAspect aspect{ ImageAspect::IMAGE_ASPECT_COLOR };
	u32 mipLevels{ 1 };
};

struct SamplerSpecification
{
	Filter minFiltering{ Filter::FILTER_LINEAR };
	Filter magFiltering{ Filter::FILTER_LINEAR };
	SamplerMipMapMode mipmapFiltering{ SamplerMipMapMode::SAMPLER_MIPMAP_MODE_LINEAR };
	SamplerAddressMode addressMode{ SamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT };
	float minLod{ 0.0f };
	float maxLod{ 100.0f };
	float lodBias{ 0.0f };
};



class Image
{
private:

public:
	virtual ~Image() {}

	virtual void SetLayout(ImageLayout newLayout, AccessFlag srcAccess, AccessFlag dstAccess, PipelineStage srcStage, PipelineStage dstStage) = 0;

	virtual const ImageSpecification& GetSpecification() const = 0;

	virtual void SetCurrentLayout(ImageLayout layout) = 0;

	virtual void CopyToImage(Image* dst) = 0;
};


class Sampler
{
private:
	
public:
	virtual ~Sampler() {}
};


class VulkanBase;
class ImageManager
{
private:
	VulkanBase& _vulkanBase;

public:

	ImageManager(VulkanBase& vulkanBase);

	std::unique_ptr<Image>	 CreateImage(const ImageSpecification& spec)	 const;
	std::unique_ptr<Sampler> CreateSampler(const SamplerSpecification& spec) const;
};

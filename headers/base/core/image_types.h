#pragma once
#include "../../util/util.h"

enum class ImageType : u8
{
	IMAGE_TYPE_NONE,
	IMAGE_TYPE_SWAPCHAIN,
	IMAGE_TYPE_TEXTURE,
	IMAGE_TYPE_RENDER_TARGET,
	IMAGE_TYPE_DEPTH_BUFFER,
};


enum class ImageFormat : u8
{
	IMAGE_FORMAT_R8G8B8A8_SRGB,
	IMAGE_FORMAT_B8G8R8A8_SRGB,
	IMAGE_FORMAT_R16G16B16A16_SFLOAT,
	IMAGE_FORMAT_D32_SFLOAT,
	IMAGE_FORMAT_R32G32_UINT,
};

enum class ImageUsage : u32
{
	IMAGE_USAGE_COLOR_ATTACHMENT = 0x00000001,
	IMAGE_USAGE_SAMPLED = 0x00000002,
	IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 0x00000004,
	IMAGE_USAGE_STORAGE_BIT = 0x00000008,
};

inline bool operator&(ImageUsage fst, ImageUsage scd)
{
	return (static_cast<u32>(fst) & static_cast<u32>(scd)) != 0;
}

inline ImageUsage operator|(ImageUsage fst, ImageUsage scd)
{
	return static_cast<ImageUsage>(static_cast<u32>(fst) | static_cast<u32>(scd));
}

enum class ImageLayout : u8
{
	IMAGE_LAYOUT_UNDEFINED,
	IMAGE_LAYOUT_GENERAL,
	IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
	IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
	IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	IMAGE_LAYOUT_PREINITIALIZED,
};

enum class ImageAspect : u8
{
	IMAGE_ASPECT_COLOR,
	IMAGE_ASPECT_DEPTH,

};

struct ImageExtent2D
{
	u32 x{ 0 };
	u32 y{ 0 };
};

struct ImageExtent3D
{
	u32 x{ 0 };
	u32 y{ 0 };
	u32 z{ 0 };
};



/*
 .|'''.|      |     '||    ||' '||''|.  '||'      '||''''|  '||''|.
 ||..  '     |||     |||  |||   ||   ||  ||        ||  .     ||   ||
  ''|||.    |  ||    |'|..'||   ||...|'  ||        ||''|     ||''|'
.     '||  .''''|.   | '|' ||   ||       ||        ||        ||   |.
|'....|'  .|.  .||. .|. | .||. .||.     .||.....| .||.....| .||.  '|'

*/

enum class Filter : u8
{
	FILTER_NEAREST,
	FILTER_LINEAR,
};

enum class SamplerMipMapMode : u8
{
	SAMPLER_MIPMAP_MODE_NEAREST,
	SAMPLER_MIPMAP_MODE_LINEAR,
};

enum class SamplerAddressMode : u8
{
	SAMPLER_ADDRESS_MODE_REPEAT,
};

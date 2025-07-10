#pragma once
#include "../../util/util.h"
#include "image_types.h"

enum class PipelineType : u8
{
	UNDEFINED,
	GRAPHICS_PIPELINE,
	COMPUTE_PIPELINE,

};


enum class AccessFlag : u32
{
	NONE = 0,
	INDEX_READ = 0x00000002,
	VERTEX_ATTRIBUTE_READ = 0x00000004,
	UNIFORM_READ = 0x00000008,
	INPUT_ATTACHMENT_READ = 0x00000010,
	SHADER_READ = 0x00000020,
	SHADER_WRITE = 0x00000040,
	COLOR_ATTACHMENT_READ = 0x00000080,
	COLOR_ATTACHMENT_WRITE = 0x00000100,
	DEPTH_STENCIL_ATTACHMENT_READ = 0x00000200,
	DEPTH_STENCIL_ATTACHMENT_WRITE = 0x00000400,
	TRANSFER_READ = 0x00000800,
	TRANSFER_WRITE = 0x00001000,
	HOST_READ = 0x00002000,
	HOST_WRITE = 0x00004000,
	MEMORY_READ = 0x00008000,
	MEMORY_WRITE = 0x00010000,
};

inline bool operator&(AccessFlag fst, AccessFlag scd)
{
	return (static_cast<u32>(fst) & static_cast<u32>(scd)) != 0;
}

inline AccessFlag operator|(AccessFlag fst, AccessFlag scd)
{
	return static_cast<AccessFlag>(static_cast<u32>(fst) | static_cast<u32>(scd));
}


enum class PipelineStage : u32
{
	TOP_OF_PIPE = 0x00000001,
	DRAW_INDIRECT = 0x00000002,
	VERTEX_SHADER = 0x00000008,
	FRAGMENT_SHADER = 0x00000080,
	EARLY_FRAGMENT_TESTS = 0x00000100,
	LATE_FRAGMENT_TESTS = 0x00000200,
	COLOR_ATTACHMENT_OUTPUT = 0x00000400,
	COMPUTE_SHADER = 0x00000800,
	ALL_TRANSFER = 0x00001000,
	BOTTOM_OF_PIPE = 0x00002000,
};

inline bool operator&(PipelineStage fst, PipelineStage scd)
{
	return (static_cast<u32>(fst) & static_cast<u32>(scd)) != 0;
}

inline PipelineStage operator|(PipelineStage fst, PipelineStage scd)
{
	return static_cast<PipelineStage>(static_cast<u32>(fst) | static_cast<u32>(scd));
}

enum class PrimitiveTopology : u8
{
	PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
};

enum class PolygonMode : u8
{
	POLYGON_MODE_FILL,
};

enum class CullMode : u8
{
	CULL_MODE_NONE,
	CULL_MODE_BACK,
	CULL_MODE_FRONT,
};

enum class FrontFace : u8
{
	FRONT_FACE_CCW,
	FRONT_FACE_CW,
};

enum class CompareOP : u8
{
	COMPARE_OP_GREATER,
	COMPARE_OP_LESS,
};

// PIPELINE BARRIER
// To do: abstract vulkan
class Image;
struct PipelineImageBarrierInfo
{
	PipelineStage srcStageMask{ PipelineStage::TOP_OF_PIPE };
	PipelineStage dstStageMask{ PipelineStage::TOP_OF_PIPE };
	AccessFlag    srcAccessMask{ AccessFlag::NONE };
	AccessFlag    dstAccessMask{ AccessFlag::NONE };

	Image* image{ nullptr };
	ImageLayout newLayout{ ImageLayout::IMAGE_LAYOUT_UNDEFINED };

	ImageAspect aspect{ ImageAspect::IMAGE_ASPECT_COLOR };
};

struct PipelineMemoryBarrierInfo
{
	PipelineStage srcStageMask{ PipelineStage::TOP_OF_PIPE };
	PipelineStage dstStageMask{ PipelineStage::TOP_OF_PIPE };
	AccessFlag    srcAccessMask{ AccessFlag::NONE };
	AccessFlag    dstAccessMask{ AccessFlag::NONE };
};


struct PipelineBarrierStorage
{
	std::vector<PipelineImageBarrierInfo> imageBarriers;
	std::vector<PipelineMemoryBarrierInfo> memoryBarriers;
};
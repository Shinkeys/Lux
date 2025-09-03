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
	INDEX_READ = 1 << 0,
	VERTEX_ATTRIBUTE_READ = 1 << 1,
	UNIFORM_READ = 1 << 2,
	INPUT_ATTACHMENT_READ = 1 << 3,
	SHADER_READ = 1 << 4, 
	SHADER_WRITE = 1 << 5,
	COLOR_ATTACHMENT_READ = 1 << 6,
	COLOR_ATTACHMENT_WRITE = 1 << 7,
	DEPTH_STENCIL_ATTACHMENT_READ = 1 << 8,
	DEPTH_STENCIL_ATTACHMENT_WRITE = 1 << 9, 
	TRANSFER_READ = 1 << 10,
	TRANSFER_WRITE = 1 << 11,
	HOST_READ = 1 << 12,
	HOST_WRITE = 1 << 13,
	MEMORY_READ = 1 << 14, 
	MEMORY_WRITE = 1 << 15, 
	ACCELERATION_READ = 1 << 16,
	ACCELERATION_WRITE = 1 << 17,
	STORAGE_READ = 1 << 18,
	STORAGE_WRITE = 1 << 19
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
	TOP_OF_PIPE = 1 << 0,
	DRAW_INDIRECT = 1 << 1,
	VERTEX_SHADER = 1 << 2, 
	FRAGMENT_SHADER = 1 << 3,
	EARLY_FRAGMENT_TESTS = 1 << 4,
	LATE_FRAGMENT_TESTS = 1 << 5,
	COLOR_ATTACHMENT_OUTPUT = 1 << 6, 
	COMPUTE_SHADER = 1 << 7,
	ALL_TRANSFER = 1 << 8,
	BOTTOM_OF_PIPE = 1 << 9,
	ACCELERATION_BUILD = 1 << 10,  
	RAY_TRACING_SHADER = 1 << 11
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
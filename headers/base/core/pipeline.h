#pragma once
#include "../../util/util.h"
#include "../../util/gfx/vk_types.h"
#include "pipeline_types.h"
#include "image_types.h"
#include "descriptor.h"





// Type shader name without extensions ---
// Incorrect: shading.vert
// Correct: shading
struct PipelineSpecification
{
	PipelineType type{ PipelineType::UNDEFINED };

	std::filesystem::path shaderName{};
	std::vector<std::string> entryPoints;

	PrimitiveTopology topology{ PrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
	PolygonMode polygonMode{ PolygonMode::POLYGON_MODE_FILL };
	CullMode cullMode{ CullMode::CULL_MODE_NONE };
	FrontFace frontFace{ FrontFace::FRONT_FACE_CCW };
	bool depthTestEnable { true };
	bool depthWriteEnable{ true };
	CompareOP depthCompare{ CompareOP::COMPARE_OP_GREATER };

	std::vector<ImageFormat> colorFormats;
	ImageFormat depthFormat{ ImageFormat::IMAGE_FORMAT_D32_SFLOAT };

	u32 attachmentsCount{ 1 };
	
	std::vector<Descriptor*> descriptorSets;

	u32 pushConstantSizeBytes{ 0 };
	u32 pushConstantOffset{ 0 };
};

class Pipeline
{
private:

public:
	virtual ~Pipeline() { }

	virtual const PipelineSpecification& GetSpecification() = 0;

};

class VulkanBase;
class PipelineManager
{
private:
	VulkanBase& _vulkanBase;


public:
	PipelineManager(VulkanBase& vulkanBase);


	void Cleanup();

	std::unique_ptr<Pipeline> CreatePipeline(const PipelineSpecification& spec);
};
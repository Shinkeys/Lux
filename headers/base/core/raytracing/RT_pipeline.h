#pragma once
#include "../../../util/util.h"

enum class RTShaderGroup
{
	SHADER_STAGE_RAYGEN,
	SHADER_STAGE_MISS,
	SHADER_STAGE_CLOSEST_HIT,
};

struct RTShaderSpec
{
	std::filesystem::path name{};
	std::string entryPoint{};
	RTShaderGroup shaderGroup{};
};

class Descriptor;
struct RTPipelineSpecification
{
	std::vector<RTShaderSpec> shaders;

	u32 pushConstantSizeBytes{ 0 };
	u32 pushConstantOffset{ 0 };

	std::vector<Descriptor*> descriptorSets;
};

class RTPipeline
{
private:


public:

	virtual ~RTPipeline() {}

};
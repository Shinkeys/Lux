#include "../../../../headers/base/gfx/raytracing/vk_rt_pipeline.h"
#include "../../../../headers/base/gfx/vk_shader.h"
#include "../../../../headers/base/gfx/vk_descriptor.h"
#include "../../../../headers/base/gfx/vk_deleter.h"
#include "../../../../headers/base/core/descriptor.h"

VulkanRTPipeline::VulkanRTPipeline(const RTPipelineSpecification& spec, VulkanDevice& deviceObj, VulkanShader& shaderObj) :
	_specification{ spec }, _deviceObject { deviceObj }, _shaderObject{ shaderObj }
{
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (u32 i = 0; i < spec.shaders.size(); ++i)
	{
		fs::path shaderPath = spec.shaders[i].name;

		shaderPath += ".slang";

		slang::IModule* slangModule = _shaderObject.LoadModule(shaderPath);

		if (spec.shaders[i].entryPoint.empty())
		{
			std::cout << "Entrypoints for shader are empty: " << shaderPath.string() << '\n';
			assert(false);
		}

		VkShaderModule shaderModule = _shaderObject.CreateShaderModule(slangModule, spec.shaders[i].entryPoint);



		switch (spec.shaders[i].shaderGroup)
		{
		case RTShaderGroup::SHADER_STAGE_RAYGEN:
		{
			const u32 stageIndex = static_cast<u32>(shaderStages.size());
			VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			group.generalShader = stageIndex;
			group.anyHitShader = VK_SHADER_UNUSED_KHR;
			group.closestHitShader = VK_SHADER_UNUSED_KHR;
			group.intersectionShader = VK_SHADER_UNUSED_KHR;
			shaderGroups.push_back(group);


			VkPipelineShaderStageCreateInfo              stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			stage.pName = "main";
			stage.module = shaderModule;
			stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			shaderStages.push_back(stage);
			break;
		}

		case RTShaderGroup::SHADER_STAGE_MISS:
		{
			const u32 stageIndex = static_cast<u32>(shaderStages.size());
			VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			group.anyHitShader = VK_SHADER_UNUSED_KHR;
			group.closestHitShader = VK_SHADER_UNUSED_KHR;
			group.intersectionShader = VK_SHADER_UNUSED_KHR;
			group.generalShader = stageIndex;
			shaderGroups.push_back(group);


			VkPipelineShaderStageCreateInfo              stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			stage.pName = "main";
			stage.module = shaderModule;
			stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
			shaderStages.push_back(stage);
			break;
		}

		case RTShaderGroup::SHADER_STAGE_CLOSEST_HIT:
		{
			const u32 stageIndex = static_cast<u32>(shaderStages.size());
			VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			group.anyHitShader = VK_SHADER_UNUSED_KHR;
			group.intersectionShader = VK_SHADER_UNUSED_KHR;
			group.closestHitShader = stageIndex;
			shaderGroups.push_back(group);

			VkPipelineShaderStageCreateInfo              stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			stage.pName = "main";
			stage.module = shaderModule;
			stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			shaderStages.push_back(stage);
			break;
		}

		default: std::unreachable();

		}

	}

	VkPushConstantRange pcRange;
	pcRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR; // To Do: other st.
	pcRange.size = spec.pushConstantSizeBytes;
	pcRange.offset = spec.pushConstantOffset;

	std::vector<VkDescriptorSetLayout> descLayouts;
	for (u32 i = 0; i < spec.descriptorSets.size(); ++i)
	{
		VulkanDescriptor* rawDescriptor =
			static_cast<VulkanDescriptor*>(spec.descriptorSets[i]);

		assert(rawDescriptor && "Passed nullptr descriptor for graphics pipeline creation");

		descLayouts.push_back(rawDescriptor->GetRawSetLayout());
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = static_cast<u32>(descLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descLayouts.empty() ? nullptr : descLayouts.data();

	if (spec.pushConstantSizeBytes > 0)
	{
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pcRange;
	}

	VK_CHECK(vkCreatePipelineLayout(deviceObj.GetDevice(), &pipelineLayoutInfo, nullptr, &_layout));

	Logger::Log("[PIPELINE] Created ray tracing pipeline layout", _layout, LogLevel::Debug);

	VkRayTracingPipelineCreateInfoKHR pipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
	pipelineInfo.groupCount = static_cast<u32>(shaderGroups.size()); // raygen, miss, one hit and others
	pipelineInfo.pGroups = shaderGroups.data();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.stageCount = static_cast<u32>(shaderStages.size());
	pipelineInfo.maxPipelineRayRecursionDepth = 1; // TO VERIFY MAYBE NEEDS TO BE MORE
	pipelineInfo.layout = _layout;

	VK_CHECK(vkCreateRayTracingPipelinesKHR(deviceObj.GetDevice(), {}, {}, 1, & pipelineInfo, nullptr, &_pipeline));

	Logger::Log("[PIPELINE] Created ray tracing pipeline", _layout, LogLevel::Debug);

	VkDevice vkDevice = deviceObj.GetDevice();
	for (auto& stage : shaderStages)
	{
		vkDestroyShaderModule(vkDevice, stage.module, nullptr);
	}
}

VulkanRTPipeline::~VulkanRTPipeline()
{
	VkDevice device = _deviceObject.GetDevice();
	VkPipeline pipeline = _pipeline;
	VkPipelineLayout layout = _layout;

	VulkanDeleter::SubmitObjectDesctruction([device, pipeline, layout]() {
		vkDestroyPipelineLayout(device, layout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		});
}
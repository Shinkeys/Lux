#include "../../../headers/base/gfx/vk_pipeline.h"
#include "../../../headers/util/gfx/vk_helpers.h"
#include "../../../headers/base/gfx/vk_descriptor.h"
#include "../../../headers/base/gfx/vk_image.h"

VulkanPipeline::VulkanPipeline(const PipelineSpecification& spec, VulkanDevice& deviceObj) :
	_specification{ spec },	  _deviceObject { deviceObj }
{
	switch (spec.type)
	{
	case PipelineType::UNDEFINED: assert(false && "Trying to create pipeline with unspecified type");
		break;

	case PipelineType::GRAPHICS_PIPELINE:
		CreateGraphicsPipeline();
		break;

	case PipelineType::COMPUTE_PIPELINE:
		CreateComputePipeline();
		break;
	}

}

void VulkanPipeline::CreateGraphicsPipeline()
{
	const VkDevice device = _deviceObject.GetDevice();

	fs::path vertexPath   = _specification.shaderName;
	fs::path fragmentPath = _specification.shaderName;

	vertexPath += ".vert.spv";
	fragmentPath += ".frag.spv";

	auto vertShaderModule = vkhelpers::ReadShaderFile(vertexPath, device);
	auto fragShaderModule = vkhelpers::ReadShaderFile(fragmentPath, device);

	Logger::Log("Cannot create vertex shader module which is required in graphics pipeline", vertShaderModule.has_value(), LogLevel::Fatal);

	// Assign shaders to the specific pipeline stage
	// Vertex shader
	VkPipelineShaderStageCreateInfo vertStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStageInfo.module = vertShaderModule.value();
	vertStageInfo.pName = "main"; // Entrypoint
	// Very useful feature - use it later. Allows you to specify
	// all the constants which would be used in the shader directly
	// from the CPU so you don't need to synchronize their values,
	// as well as the driver can optimize them to reduce branching
	vertStageInfo.pSpecializationInfo = nullptr;
	

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages
	{
		vertStageInfo
	};

	if (fragShaderModule.has_value())
	{
		// Fragment shader
		VkPipelineShaderStageCreateInfo fragStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStageInfo.module = fragShaderModule.value();
		fragStageInfo.pName = "main";
		fragStageInfo.pSpecializationInfo = nullptr; // To do

		shaderStages.push_back(fragStageInfo);
	}


	// To do
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	// To do
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyInfo.topology = vkconversions::ToVkPrimitiveTopology(_specification.topology);
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	// To do
	// Viewport and rect might be dynamic.
	const VkViewport nullViewport{};
	const VkRect2D nullScissor{};

	VkPipelineViewportStateCreateInfo viewportStateInfo{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	viewportStateInfo.pViewports = &nullViewport;
	viewportStateInfo.pScissors  = &nullScissor;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.scissorCount  = 1;

	// Dynamic state
	std::vector<VkDynamicState> dynamicStates
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateInfo.dynamicStateCount = static_cast<u32>(dynamicStates.size());
	dynamicStateInfo.pDynamicStates = dynamicStates.data();

	// To do
	VkPipelineRasterizationStateCreateInfo rasterizationInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationInfo.depthClampEnable = VK_FALSE; // if true values beyond the near and far planes are not discarded
	rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationInfo.polygonMode = vkconversions::ToVkPolygonMode(_specification.polygonMode);
	rasterizationInfo.cullMode    = vkconversions::ToVkCullMode(_specification.cullMode);
	rasterizationInfo.frontFace   = vkconversions::ToVkFrontFace(_specification.frontFace);
	rasterizationInfo.depthBiasEnable = VK_FALSE;
	rasterizationInfo.depthBiasConstantFactor = 0.0f;
	rasterizationInfo.depthBiasClamp = 0.0f;
	rasterizationInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationInfo.lineWidth = 1.0f;


	// To do
	VkPipelineMultisampleStateCreateInfo multisamplingInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisamplingInfo.sampleShadingEnable  = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingInfo.minSampleShading = 1.0f;
	multisamplingInfo.pSampleMask = nullptr;
	multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingInfo.alphaToOneEnable = VK_FALSE;

	// Color blending
	// Blend color from frag shader and framebuffer
	// color.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor
	// color.alpha = newAlpha.a
	VkPipelineColorBlendAttachmentState colorBlendState{ .blendEnable = VK_TRUE };
	colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlends;
	for (u32 i = 0; i < _specification.attachmentsCount; ++i)
		colorBlends.push_back(colorBlendState);

	VkPipelineColorBlendStateCreateInfo colorBlendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendInfo.attachmentCount = _specification.attachmentsCount;
	colorBlendInfo.pAttachments = colorBlends.data();
	colorBlendInfo.blendConstants[0] = 0.0f;
	colorBlendInfo.blendConstants[1] = 0.0f;
	colorBlendInfo.blendConstants[2] = 0.0f;
	colorBlendInfo.blendConstants[3] = 0.0f;

	VkPushConstantRange pushConstant;
	pushConstant.size   = _specification.pushConstantSizeBytes;
	pushConstant.offset = _specification.pushConstantOffset;
	pushConstant.stageFlags = VK_SHADER_STAGE_ALL;


	std::vector<VkDescriptorSetLayout> descLayouts;
	for (u32 i = 0; i < _specification.descriptorSets.size(); ++i)
	{
		VulkanDescriptor* rawDescriptor = 
			static_cast<VulkanDescriptor*>(_specification.descriptorSets[i]);

		assert(rawDescriptor && "Passed nullptr descriptor for graphics pipeline creation");

		descLayouts.push_back(rawDescriptor->GetRawSetLayout());
	}


	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = static_cast<u32>(_specification.descriptorSets.size());
	pipelineLayoutInfo.pSetLayouts = descLayouts.empty() ?  nullptr : descLayouts.data();

	if (_specification.pushConstantSizeBytes > 0)
	{
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	}

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_layout));

	Logger::Log("[PIPELINE] Created pipeline layout", _layout, LogLevel::Debug);


	std::vector<VkFormat> colorFormats;
	for (u32 i = 0; i < _specification.colorFormats.size(); ++i)
	{
		colorFormats.push_back(vkconversions::ToVkFormat(_specification.colorFormats[i]));
	}

	VkPipelineRenderingCreateInfo pipelineRenderingInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO }; // for dynamic rendering
	pipelineRenderingInfo.colorAttachmentCount = _specification.attachmentsCount;
	pipelineRenderingInfo.pColorAttachmentFormats = colorFormats.empty() ? nullptr : colorFormats.data();
	pipelineRenderingInfo.depthAttachmentFormat = vkconversions::ToVkFormat(_specification.depthFormat);

	// to do: VkPipelineDepthStencilStateCreateInfo 
	VkPipelineDepthStencilStateCreateInfo depthState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthState.depthTestEnable = _specification.depthTestEnable   ? VK_TRUE : VK_FALSE;
	depthState.depthWriteEnable = _specification.depthWriteEnable ? VK_TRUE : VK_FALSE;
	depthState.depthCompareOp = vkconversions::ToVkCompareOP(_specification.depthCompare);
	depthState.depthBoundsTestEnable = VK_FALSE;
	depthState.stencilTestEnable = VK_FALSE;

	VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.pNext = &pipelineRenderingInfo;
	pipelineInfo.stageCount = static_cast<u32>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizationInfo;
	pipelineInfo.pMultisampleState = &multisamplingInfo;
	pipelineInfo.pDepthStencilState = &depthState;
	pipelineInfo.pColorBlendState = fragShaderModule.has_value() ? &colorBlendInfo : nullptr;
	pipelineInfo.pDynamicState = &dynamicStateInfo;
	pipelineInfo.layout = _layout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;


	VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline));

	Logger::Log("[PIPELINE] Created pipeline object", _pipeline, LogLevel::Debug);

	vkDestroyShaderModule(device, vertShaderModule.value(), nullptr);

	if(fragShaderModule.has_value())
		vkDestroyShaderModule(device, fragShaderModule.value(), nullptr);
}

	
void VulkanPipeline::CreateComputePipeline()
{
	const VkDevice device = _deviceObject.GetDevice();

	fs::path computePath = _specification.shaderName;

	computePath += ".comp.spv";

	auto compShaderModule = vkhelpers::ReadShaderFile(computePath, device);

	Logger::Log("Cannot create vertex shader module which is required in graphics pipeline", compShaderModule.has_value(), LogLevel::Fatal);


	VkPipelineShaderStageCreateInfo shaderStageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStageInfo.module = compShaderModule.value();
	shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.pName = "main"; // enty point


	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = static_cast<u32>(_specification.descriptorSets.size());

	std::vector<VkDescriptorSetLayout> descLayouts;
	for (u32 i = 0; i < _specification.descriptorSets.size(); ++i)
	{
		VulkanDescriptor* rawDescriptor =
			static_cast<VulkanDescriptor*>(_specification.descriptorSets[i]);

		assert(rawDescriptor && "Passed nullptr descriptor for compute pipeline creation");

		descLayouts.push_back(rawDescriptor->GetRawSetLayout());
	}

	pipelineLayoutInfo.pSetLayouts = descLayouts.empty() ? nullptr : descLayouts.data();

	VkPushConstantRange pushConstant;
	pushConstant.size   = _specification.pushConstantSizeBytes;
	pushConstant.offset = _specification.pushConstantOffset;
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	if (_specification.pushConstantSizeBytes > 0)
	{
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	}

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_layout));

	Logger::Log("[PIPELINE] Created compute pipeline layout", _layout, LogLevel::Debug);



	VkComputePipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	createInfo.stage = shaderStageInfo;
	createInfo.layout = _layout;
	createInfo.basePipelineHandle = nullptr;
	createInfo.basePipelineIndex = -1;

	VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &_pipeline));

	Logger::Log("[PIPELINE] Created compute pipeline object", _pipeline, LogLevel::Debug);

	vkDestroyShaderModule(device, compShaderModule.value(), nullptr);
}





namespace vkconversions
{
	VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology topology)
	{
		switch (topology)
		{
		case PrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		default: std::unreachable();
		}
	}

	VkPolygonMode ToVkPolygonMode(PolygonMode mode)
	{
		switch (mode)
		{
		case PolygonMode::POLYGON_MODE_FILL:
			return VK_POLYGON_MODE_FILL;
		default: std::unreachable();
		}
	}

	VkCullModeFlagBits ToVkCullMode(CullMode mode)
	{
		switch (mode)
		{
		case CullMode::CULL_MODE_NONE:
			return VK_CULL_MODE_NONE;
		case CullMode::CULL_MODE_BACK:
			return VK_CULL_MODE_BACK_BIT;
		case CullMode::CULL_MODE_FRONT:
			return VK_CULL_MODE_FRONT_BIT;
		default: std::unreachable();
		}
	}

	VkFrontFace ToVkFrontFace(FrontFace face)
	{
		switch (face)
		{
		case FrontFace::FRONT_FACE_CCW:
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		case FrontFace::FRONT_FACE_CW:
			return VK_FRONT_FACE_CLOCKWISE;
		default: std::unreachable();
		}
	}

	VkCompareOp ToVkCompareOP(CompareOP op)
	{
		switch (op)
		{
		case CompareOP::COMPARE_OP_GREATER:
			return VK_COMPARE_OP_GREATER;
		case CompareOP::COMPARE_OP_LESS:
			return VK_COMPARE_OP_LESS;
		default: std::unreachable();
		}
	}

	VkAccessFlags2 ToVkAccessFlags2(AccessFlag flags)
	{
		VkAccessFlags2 result = 0;

		if (flags & AccessFlag::NONE)
			result |= VK_ACCESS_2_NONE;

		if (flags & AccessFlag::INDEX_READ)
			result |= VK_ACCESS_2_INDEX_READ_BIT;

		if (flags & AccessFlag::VERTEX_ATTRIBUTE_READ)
			result |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;

		if (flags & AccessFlag::UNIFORM_READ)
			result |= VK_ACCESS_2_UNIFORM_READ_BIT;

		if (flags & AccessFlag::INPUT_ATTACHMENT_READ)
			result |= VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT;

		if (flags & AccessFlag::SHADER_READ)
			result |= VK_ACCESS_2_SHADER_READ_BIT;

		if (flags & AccessFlag::SHADER_WRITE)
			result |= VK_ACCESS_2_SHADER_WRITE_BIT;

		if (flags & AccessFlag::COLOR_ATTACHMENT_READ)
			result |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;

		if (flags & AccessFlag::COLOR_ATTACHMENT_WRITE)
			result |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

		if (flags & AccessFlag::DEPTH_STENCIL_ATTACHMENT_READ)
			result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

		if (flags & AccessFlag::DEPTH_STENCIL_ATTACHMENT_WRITE)
			result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		if (flags & AccessFlag::TRANSFER_READ)
			result |= VK_ACCESS_2_TRANSFER_READ_BIT;

		if (flags & AccessFlag::TRANSFER_WRITE)
			result |= VK_ACCESS_2_TRANSFER_WRITE_BIT;

		if (flags & AccessFlag::HOST_READ)
			result |= VK_ACCESS_2_HOST_READ_BIT;

		if (flags & AccessFlag::HOST_WRITE)
			result |= VK_ACCESS_2_HOST_WRITE_BIT;

		if (flags & AccessFlag::MEMORY_READ)
			result |= VK_ACCESS_2_MEMORY_READ_BIT;

		if (flags & AccessFlag::MEMORY_WRITE)
			result |= VK_ACCESS_2_MEMORY_WRITE_BIT;

		return result;
	}


	VkPipelineStageFlags2 ToVkPipelineStageFlags2(PipelineStage stages)
	{
		VkPipelineStageFlags2 result = 0;

		if (stages & PipelineStage::TOP_OF_PIPE)
			result |= VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

		if (stages & PipelineStage::DRAW_INDIRECT)
			result |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;

		if (stages & PipelineStage::VERTEX_SHADER)
			result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

		if (stages & PipelineStage::FRAGMENT_SHADER)
			result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		if (stages & PipelineStage::EARLY_FRAGMENT_TESTS)
			result |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;

		if (stages & PipelineStage::LATE_FRAGMENT_TESTS)
			result |= VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

		if (stages & PipelineStage::COLOR_ATTACHMENT_OUTPUT)
			result |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		if (stages & PipelineStage::COMPUTE_SHADER)
			result |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

		if (stages & PipelineStage::ALL_TRANSFER)
			result |= VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;

		if (stages & PipelineStage::BOTTOM_OF_PIPE)
			result |= VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

		if (result == 0)
			assert(false && "PipelineStage2 conversion is not implemented for Vulkan");

		return result;
	}
}
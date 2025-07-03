#include "../../../headers/base/gfx/vk_pipeline.h"
#include "../../../headers/util/gfx/vk_helpers.h"

VulkanPipeline::VulkanPipeline(VulkanDevice& deviceObj, VulkanPresentation& presentationObj) : 
				_deviceObject{deviceObj}, _presentationObject{presentationObj}
{

}
const PipelinePair* VulkanPipeline::GetPipelinePair(const GraphicsPipeline& graphicsPipeline) const
{
	auto it = _pipelinesCache.find(graphicsPipeline);

	if (it == _pipelinesCache.end())
	{
		Logger::CriticalLog("Can't find pipeline layout by passed graphics pipeline. Check call stack");
		return nullptr;
	}

	return &it->second;
}

PipelinePair VulkanPipeline::CreatePipeline(const GraphicsPipeline& graphicsPipeline)
{
	if (auto it = _pipelinesCache.find(graphicsPipeline); it != _pipelinesCache.end())
	{
		return it->second;
	}

	const VkDevice device = _deviceObject.GetDevice();

	fs::path vertexPath = graphicsPipeline.shaderName;
	fs::path fragmentPath = graphicsPipeline.shaderName;

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
	inputAssemblyInfo.topology = graphicsPipeline.topology;
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
	rasterizationInfo.polygonMode = graphicsPipeline.polygonMode;
	rasterizationInfo.cullMode  = graphicsPipeline.cullMode;
	rasterizationInfo.frontFace = graphicsPipeline.frontFace;
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

	VkPipelineColorBlendStateCreateInfo colorBlendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &colorBlendState;
	colorBlendInfo.blendConstants[0] = 0.0f;
	colorBlendInfo.blendConstants[1] = 0.0f;
	colorBlendInfo.blendConstants[2] = 0.0f;
	colorBlendInfo.blendConstants[3] = 0.0f;

	VkPushConstantRange pushConstant;
	pushConstant.size = graphicsPipeline.pushConstantSizeBytes;
	pushConstant.offset = graphicsPipeline.pushConstantOffset;
	pushConstant.stageFlags = VK_SHADER_STAGE_ALL;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = static_cast<u32>(graphicsPipeline.descriptorLayouts.size());
	pipelineLayoutInfo.pSetLayouts = graphicsPipeline.descriptorLayouts.size() > 0 ? graphicsPipeline.descriptorLayouts.data() : nullptr;

	if (graphicsPipeline.pushConstantSizeBytes > 0)
	{
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	}

	PipelinePair pipelinePair;

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelinePair.pipelineLayout));

	Logger::Log("[PIPELINE] Created pipeline layout", pipelinePair.pipelineLayout, LogLevel::Debug);

	VkPipelineRenderingCreateInfo pipelineRenderingInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO }; // for dynamic rendering
	pipelineRenderingInfo.colorAttachmentCount = 1;
	pipelineRenderingInfo.pColorAttachmentFormats = &graphicsPipeline.colorFormat;
	pipelineRenderingInfo.depthAttachmentFormat = graphicsPipeline.depthFormat;

	// to do: VkPipelineDepthStencilStateCreateInfo 
	VkPipelineDepthStencilStateCreateInfo depthState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthState.depthTestEnable = graphicsPipeline.depthTestEnable;
	depthState.depthWriteEnable = graphicsPipeline.depthWriteEnable;
	depthState.depthCompareOp = graphicsPipeline.depthCompare;
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
	pipelineInfo.layout = pipelinePair.pipelineLayout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;



	VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipelinePair.pipeline));

	Logger::Log("[PIPELINE] Created pipeline object", pipelinePair.pipeline, LogLevel::Debug);

	// Store it in the storage
	_pipelinesCache.insert({ graphicsPipeline, pipelinePair });


	vkDestroyShaderModule(device, vertShaderModule.value(), nullptr);

	if(fragShaderModule.has_value())
		vkDestroyShaderModule(device, fragShaderModule.value(), nullptr);

	return pipelinePair;
}

void VulkanPipeline::SetDynamicStates(VkCommandBuffer cmdBuffer) const
{
	const VkExtent2D swapchainExtent = _presentationObject.GetSwapchainDesc().extent;
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = static_cast<float>(swapchainExtent.height);
	viewport.width  = static_cast<float>(swapchainExtent.width);
	viewport.height = -static_cast<float>(swapchainExtent.height); // Flipping viewport to change Y positive direction
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainExtent;
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
}

void VulkanPipeline::Cleanup()
{
	VkDevice device = _deviceObject.GetDevice();

	for (const auto& pipelinePair : _pipelinesCache)
	{
		vkDestroyPipeline(device, pipelinePair.second.pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelinePair.second.pipelineLayout, nullptr);
	}
}
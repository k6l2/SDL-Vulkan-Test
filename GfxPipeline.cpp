#include "GfxPipeline.h"
#include "RenderWindow.h"
k10::GfxPipeline::GfxPipeline(VkDevice d)
	: device(d)
{
}
k10::GfxPipeline::~GfxPipeline()
{
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}
bool k10::GfxPipeline::createPipelineLayout(
	VkExtent2D swapChainExtent,
	vector<VkPipelineShaderStageCreateInfo> const& shaderStages,
	VkRenderPass renderPass)
{
	VkViewport viewport = {
		0.f,// x
		0.f,// y
		static_cast<float>(swapChainExtent.width),
		static_cast<float>(swapChainExtent.height),
		0.f,// depth min
		1.f // depth max
	};
	VkRect2D scissor = {
		{0, 0},// offset
		swapChainExtent // extent
	};
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		1,// viewport count
		& viewport,
		1,// scissor count
		& scissor
	};
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		0,// vertexBindingDescriptionCount
		nullptr,// vertexBindingDescriptions
		0,// vertexAttributeDescriptionCount
		nullptr // vertexAttributeDescriptions
	};
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		///TODO: change this to triangle strip to see what will happen:
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE // primitiveRestartEnable
	};
	VkPipelineRasterizationStateCreateInfo rasterizerStateCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		VK_FALSE,// depth clamp
		VK_FALSE,// discard enable (disables output to framebuffer)
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_BACK_BIT,
		VK_FRONT_FACE_CLOCKWISE,
		VK_FALSE,// depth bias
		0.f,// depth bias factor
		0.f,// depth bias clamp
		0.f,// depth bias slope factor
		1.f // line width
	};
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		VK_SAMPLE_COUNT_1_BIT,
		VK_FALSE,// sample shading
		1.f,// min sample shading
		nullptr,// sample mask
		VK_FALSE,// alpha to converge
		VK_FALSE // alpha to one
	};
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		VK_FALSE,// blending
		VK_BLEND_FACTOR_ONE,// src color factor
		VK_BLEND_FACTOR_ZERO,// dst color factor
		VK_BLEND_OP_ADD,// color op
		VK_BLEND_FACTOR_ONE,// src alpha factor
		VK_BLEND_FACTOR_ZERO,// dst alpha factor
		VK_BLEND_OP_ADD,// alpha op
		VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT
	};
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		VK_FALSE,// logic op enabled
		VK_LOGIC_OP_COPY,
		1,// attachment count
		&colorBlendAttachment,
		{0.f, 0.f, 0.f, 0.f}
	};
	///TODO: dynamic states?
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		0,// setLayoutCount
		nullptr,// setLayouts
		0,// pushConstantRangeCount
		nullptr // pushConstantRanges
	};
	if(vkCreatePipelineLayout(device, 
							  &pipelineLayoutCreateInfo, 
							  nullptr, 
							  &pipelineLayout) != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create pipeline layout!\n");
		SDL_assert(false);
		return false;
	}
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		static_cast<uint32_t>(shaderStages.size()),
		shaderStages.data(),
		&vertexInputStateCreateInfo,
		&inputAssemblyStateCreateInfo,
		nullptr,// tessellation state
		&viewportStateCreateInfo,
		&rasterizerStateCreateInfo,
		&multisampleStateCreateInfo,
		nullptr,// depth stencil state
		&colorBlendStateCreateInfo,
		nullptr,// dynamic state
		pipelineLayout,
		renderPass,
		0,// subpass
		VK_NULL_HANDLE,// base pipeline handle
		-1 // base pipeline index
	};
	if(vkCreateGraphicsPipelines(device, 
								 VK_NULL_HANDLE, 
								 1, &pipelineCreateInfo, 
								 nullptr, 
								 &pipeline) != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create pipeline!\n");
		SDL_assert(false);
		return false;
	}
	return true;
}
VkPipeline k10::GfxPipeline::getPipeline() const
{
	return pipeline;
}
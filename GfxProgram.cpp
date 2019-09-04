#include "GfxProgram.h"
#include "RenderWindow.h"
k10::GfxProgram::GfxProgram(VkDevice d, ShaderType st)
	: device(d)
	, shaderType(st)
{
}
k10::GfxProgram::~GfxProgram()
{
	if (shaderModuleCreated)
	{
		vkDestroyShaderModule(device, shaderModule, nullptr);
	}
}
bool k10::GfxProgram::loadFromFile(string const& spirvShaderFileName, string const& entryPoint)
{
	vector<Uint8> spirvShaderCode = readFile(spirvShaderFileName);
	VkShaderModuleCreateInfo shaderCreateInfo = {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		spirvShaderCode.size(),
		reinterpret_cast<uint32_t const*>(spirvShaderCode.data())
	};
	if (vkCreateShaderModule(device,
							 &shaderCreateInfo,
							 nullptr,
							 &shaderModule) != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create Vulkan shader module!\n");
		SDL_assert(false);
		return false;
	}
	// setup a shader stage struct for Vulkan pipeline construction later //
	programEntryPoint = entryPoint;
	shaderStageCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		getShaderStageFlagBits(),
		shaderModule,
		programEntryPoint.c_str(),
		nullptr // VkSpecializationInfo const*
	};
	shaderModuleCreated = true;
	return true;
}
VkPipelineShaderStageCreateInfo 
k10::GfxProgram::getPipelineShaderStageCreateInfo() const
{
	return shaderStageCreateInfo;
}
VkShaderStageFlagBits k10::GfxProgram::getShaderStageFlagBits() const
{
	switch (shaderType)
	{
	case ShaderType::VERTEX:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderType::FRAGMENT:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}
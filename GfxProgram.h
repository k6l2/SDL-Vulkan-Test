#pragma once
namespace k10
{
	class GfxProgram
	{
	public:
		enum class ShaderType : Uint8
		{
			VERTEX,
			FRAGMENT
		};
	public:
		GfxProgram(VkDevice d, ShaderType st);
		~GfxProgram();
		bool loadFromFile(string const& spirvShaderFileName, string const& entryPoint = "main");
		VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo() const;
	private:
		VkShaderStageFlagBits getShaderStageFlagBits() const;
	private:
		VkDevice device;
		ShaderType shaderType;
		bool shaderModuleCreated = false;
		VkShaderModule shaderModule;
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
		// we need to save the string of the program entry point 
		//	for pipeline construction later //
		string programEntryPoint;
	};
}

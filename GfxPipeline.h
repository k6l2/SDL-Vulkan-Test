#pragma once
namespace k10
{
	class GfxPipeline
	{
	public:
		GfxPipeline(VkDevice d);
		~GfxPipeline();
		bool createPipelineLayout(
			VkExtent2D swapChainExtent,
			vector<VkPipelineShaderStageCreateInfo> const& shaderStages,
			VkRenderPass renderPass);
		// RenderWindow interface //
		VkPipeline getPipeline() const;
		// ////////////////////////// end RenderWindow interface //
	private:
		VkDevice device;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
	};
}

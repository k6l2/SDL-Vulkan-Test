#pragma once
namespace k10
{
	using GfxPipelineIndex = Uint8;
	class GfxPipeline
	{
	public:
		void destroy(VkDevice d);
		bool createPipeline(
			GfxPipelineIndex gpi,
			VkDevice d,
			VkExtent2D swapChainExtent,
			vector<VkPipelineShaderStageCreateInfo> const& shaderStages,
			VkRenderPass renderPass);
		bool buildPipelineFromCache(
			VkDevice d,
			VkExtent2D swapChainExtent,
			VkRenderPass renderPass);
		// RenderWindow interface //
		VkPipeline getPipeline() const;
		GfxPipelineIndex getGpi() const;
		// ////////////////////////// end RenderWindow interface //
	private:
		GfxPipelineIndex gpi;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
		vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfoCache;
	};
}

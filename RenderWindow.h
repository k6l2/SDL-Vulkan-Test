#pragma once
#ifndef NDEBUG
#define K10_ENABLE_VULKAN_VALIDATION_LAYERS
#define K10_VULKAN_DEBUG_VERBOSE
#endif
#include "GfxProgram.h"
#include "GfxPipeline.h"
namespace k10
{
	class RenderWindow
	{
	public:
		static const GfxPipelineIndex MAX_PIPELINES = 50;
		static RenderWindow* createRenderWindow(char const* title, 
			int initialWidth, int initialHeight);
	private:
		static const int MAX_FRAMES_IN_FLIGHT;
		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			vector<VkSurfaceFormatKHR> formats;
			vector<VkPresentModeKHR> presentModes;
		};
		struct QueueFamilyIndices
		{
			uint64_t graphicsFamily = std::numeric_limits<uint64_t>::max();
			uint64_t presentFamily  = std::numeric_limits<uint64_t>::max();
			bool isSuitable() const;
			vector<uint64_t> toRawVector() const
			{
				return { graphicsFamily,
						 presentFamily };
			}
			vector<uint32_t> toVkVector() const
			{
				return { static_cast<uint32_t>(graphicsFamily),
						 static_cast<uint32_t>(presentFamily) };
			}
		};
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
		static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugMessengerCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
#endif
		static bool checkPhysicalDeviceExtensionSupport(
			VkPhysicalDevice pd, vector<char const*>const& requiredExtensionNames);
	public:
		~RenderWindow();
		bool recordCommandBuffers(GfxPipelineIndex gpi);
		bool drawFrame();
		void waitForOperationsToFinish();
		void onWindowEvent(SDL_WindowEvent const& we);
		// GfxPipeline interface //
		GfxPipelineIndex createGfxPipeline(GfxProgram const* vertProgram,
										   GfxProgram const* fragProgram);
		// //////////////////////////////// end GfxPipeline interface //
		// GfxProgram interface //
		GfxProgram* createGfxProgram(GfxProgram::ShaderType st);
///		VkDevice getDevice() const;
		// /////////////////////////////// end GfxProgram interface //
	private:
		void cleanupSwapChain();
		bool rebuildSwapChain();
		bool createSwapChain();
		bool createImageViews();
		bool createRenderPass();
		bool createFramebuffers();
		bool createCommandBuffers();
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice pd) const;
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice pd) const;
		VkExtent2D chooseSwapExtent(
			VkSurfaceCapabilitiesKHR const& c) const;
		VkPresentModeKHR chooseSwapPresentMode(
			vector<VkPresentModeKHR>const& presentModes) const;
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(
			vector<VkSurfaceFormatKHR>const& formats) const;
		GfxPipeline* findGfxPipeline(GfxPipelineIndex gpi);
	private:
		SDL_Window* window = nullptr;
		bool windowResized = false;
		bool windowMinimized = false;
		VkInstance instance;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;
		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VkSwapchainKHR swapChain;
		vector<VkImage> swapChainImages;
		VkFormat swapChainFormat;
		VkExtent2D swapChainExtent;
		vector<VkImageView> swapChainImageViews;
		VkRenderPass renderPass;
		vector<VkFramebuffer> swapChainFramebuffers;
		VkCommandPool commandPool;
		vector<VkCommandBuffer> commandBuffers;
		vector<VkSemaphore> imageAvailableSemaphores;
		vector<VkSemaphore> renderFinishedSemaphores;
		vector<VkFence> frameFences;
		size_t currentFrame = 0;
		GfxPipelineIndex nextGpi = 0;
		vector<GfxPipeline> gfxPipelines;
		GfxPipelineIndex gpiRecordedCommandBuffer;
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
		VkDebugUtilsMessengerEXT debugMessenger;
#endif
	};
}

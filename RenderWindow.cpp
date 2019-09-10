#include "RenderWindow.h"
const int k10::RenderWindow::MAX_FRAMES_IN_FLIGHT = 2;
bool k10::RenderWindow::QueueFamilyIndices::isSuitable() const
{
	return graphicsFamily != std::numeric_limits<uint64_t>::max() &&
		   presentFamily  != std::numeric_limits<uint64_t>::max();
}
k10::RenderWindow* k10::RenderWindow::createRenderWindow(
	char const* title, int initialWidth, int initialHeight)
{
	RenderWindow* retVal = new RenderWindow;
	// Create the SDL Window //
	retVal->window = SDL_CreateWindow(title,
									  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
									  initialWidth, initialHeight,
									  SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (!retVal->window)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create window! error='%s'\n", SDL_GetError());
		delete retVal;
		return nullptr;
	}
	// Get list of required Vulkan extensions we need Vulkan instance for the SDL window //
	vector<char const*> requiredExtensionNames;
	{
		uint32_t extensionCount;
		if (!SDL_Vulkan_GetInstanceExtensions(retVal->window, &extensionCount, nullptr))
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to get required Vulkan extension count! error='%s'\n", SDL_GetError());
			delete retVal;
			return nullptr;
		}
		requiredExtensionNames.resize(extensionCount);
		if(!SDL_Vulkan_GetInstanceExtensions(
			retVal->window, &extensionCount, requiredExtensionNames.data()))
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to get required Vulkan extension names! error='%s'\n", SDL_GetError());
			delete retVal;
			return nullptr;
		}
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
		requiredExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		SDL_Log("We require %i Vulkan extensions to run!\n", 
			requiredExtensionNames.size());
		for (size_t e = 0; e < requiredExtensionNames.size(); e++)
		{
			char const* name = requiredExtensionNames[e];
			SDL_Log("\t'%s'\n", name);
		}
	}
	// get a list of supported Vulkan extensions //
	vector<VkExtensionProperties> extensionProperties;
	{
		uint32_t extensionCount;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		extensionProperties.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());
		SDL_Log("Available Vulkan instance extensions:\n");
		for (auto const& ep : extensionProperties)
		{
			SDL_Log("\t'%s' (version %i)\n", ep.extensionName, ep.specVersion);
		}
		for (char const* const ren : requiredExtensionNames)
		{
			bool requirementSatisfied = false;
			for (auto const& ep : extensionProperties)
			{
				if (strcmp(ep.extensionName, ren) == 0)
				{
					requirementSatisfied = true;
					break;
				}
			}
			if (!requirementSatisfied)
			{
				SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
					"Could not satisfy vulkan extension requirement! extension='%s'\n", ren);
				delete retVal;
				return nullptr;
			}
		}
	}
	const vector<char const*> requiredLayers = {
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
		"VK_LAYER_KHRONOS_validation"
#endif
	};
	if (requiredLayers.empty())
	{
		SDL_Log("No Vulkan instance layers required!\n");
	}
	else
	{
		SDL_Log("Required Vulkan instance layers:\n");
		for (char const* const rl : requiredLayers)
		{
			SDL_Log("\t'%s'\n", rl);
		}
	}
	// Get a list of all available layers //
	vector<VkLayerProperties> layerProperties;
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		layerProperties.resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());
		SDL_Log("Available Vulkan instance layers:\n");
		for (auto const& lp : layerProperties)
		{
			SDL_Log("\t'%s' (version %i)\n", lp.layerName, lp.implementationVersion);
		}
		for (char const* const rl : requiredLayers)
		{
			bool requirementSatisfied = false;
			for (auto const& lp : layerProperties)
			{
				if (strcmp(lp.layerName, rl) == 0)
				{
					requirementSatisfied = true;
					break;
				}
			}
			if (!requirementSatisfied)
			{
				SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
					"Could not satisfy vulkan layer requirement! layer='%s'\n", rl);
				delete retVal;
				return nullptr;
			}
		}
	}
	// create Vulkan instance //
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
	const VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
		VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		nullptr,// pNext
		0,// flags
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		vulkanDebugMessengerCallback,
		nullptr // userData
	};
#endif
	{
		VkApplicationInfo appInfo = {
			VK_STRUCTURE_TYPE_APPLICATION_INFO,
			nullptr,// pNext
			title,
			VK_MAKE_VERSION(1,0,0),
			"K10",
			VK_MAKE_VERSION(1,0,0),
			VK_API_VERSION_1_0
		};
		VkInstanceCreateInfo createInfo = {
			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
			(VkDebugUtilsMessengerCreateInfoEXT*)& debugUtilsMessengerCreateInfo,
#else
			nullptr,// pNext
#endif
			0,// flags
			&appInfo,
			static_cast<uint32_t>(requiredLayers.size()),
			requiredLayers.data(),
			static_cast<uint32_t>(requiredExtensionNames.size()),
			requiredExtensionNames.data()
		};
		const VkResult result = 
			vkCreateInstance(&createInfo, nullptr, &retVal->instance);
		if (result != VK_SUCCESS)
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to create Vulkan instance!\n");
			delete retVal;
			return nullptr;
		}
	}
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
	// Setup a handler for debug messages from the validation layer 
	//	so we can choose which get displayed //
	{
		PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT;
		createDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(retVal->instance, "vkCreateDebugUtilsMessengerEXT");
		if (createDebugUtilsMessengerEXT)
		{
			if (createDebugUtilsMessengerEXT(
					retVal->instance, &debugUtilsMessengerCreateInfo, 
					nullptr, &retVal->debugMessenger) != VK_SUCCESS)
			{
				SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
					"Failed to create Vulkan Debug Utils Messanger!\n");
				delete retVal;
				return nullptr;
			}
		}
		else
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to find vkCreateDebugUtilsMessengerEXT!\n");
			delete retVal;
			return nullptr;
		}
	}
#endif
	if (!SDL_Vulkan_CreateSurface(retVal->window, 
								  retVal->instance, 
								  &retVal->surface))
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create Vulkan window surface!\n");
		delete retVal;
		return nullptr;
	}
	// Choose physical device to Vulkan on //
	const vector<const char*> requiredPhysicalDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	{
		auto isPhysicalDeviceSuitable = 
			[retVal, &requiredPhysicalDeviceExtensions](VkPhysicalDevice pd)->bool
		{
			//VkPhysicalDeviceProperties deviceProperties;
			//VkPhysicalDeviceFeatures   deviceFeatures;
			//vkGetPhysicalDeviceProperties(pd, &deviceProperties);
			//vkGetPhysicalDeviceFeatures  (pd, &deviceFeatures);
			//return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
			QueueFamilyIndices qfi = retVal->findQueueFamilies(pd);
			if (!qfi.isSuitable())
			{
				return false;
			}
			const bool extensionsSupported =
				checkPhysicalDeviceExtensionSupport(
					pd, requiredPhysicalDeviceExtensions);
			if (!extensionsSupported)
			{
				return false;
			}
			SwapChainSupportDetails swapChainSupport =
				retVal->querySwapChainSupport(pd);
			if (swapChainSupport.formats.empty() ||
				swapChainSupport.presentModes.empty())
			{
				return false;
			}
			return true;
		};
		uint32_t deviceCount;
		vkEnumeratePhysicalDevices(retVal->instance, &deviceCount, nullptr);
		if (deviceCount <= 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to find physical device with Vulkan support!\n");
			delete retVal;
			return nullptr;
		}
		vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(retVal->instance, &deviceCount, physicalDevices.data());
		SDL_Log("System has %i physical devices with Vulkan support:\n", physicalDevices.size());
		for (VkPhysicalDevice const& pd : physicalDevices)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(pd, &deviceProperties);
			SDL_Log("\t'%s'\n", deviceProperties.deviceName);
		}
		for (VkPhysicalDevice const& pd : physicalDevices)
		{
			if (isPhysicalDeviceSuitable(pd))
			{
				retVal->physicalDevice = pd;
				break;
			}
		}
		if (retVal->physicalDevice == VK_NULL_HANDLE)
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to find a suitable physical device!\n");
			delete retVal;
			return nullptr;
		}
	}
	// Create a logical device to Vulkan on //
	{
		const QueueFamilyIndices qfi = 
			retVal->findQueueFamilies(retVal->physicalDevice);
		float queuePriority = 1.f;
		const vector<uint32_t> qfiValues = qfi.toVkVector();
		vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		for (size_t i = 0; i < qfiValues.size(); i++)
		{
			bool infoExists = false;
			for (size_t j = 0; j < i; j++)
			{
				// check if we've visited this raw value yet...
				if (qfiValues[j] == qfiValues[i])
				{
					infoExists = true;
					break;
				}
			}
			if (infoExists)
			{
				continue;
			}
			VkDeviceQueueCreateInfo queueCreateInfo = {
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				nullptr,// pNext
				0,// flags
				qfiValues[i],
				1,
				&queuePriority
			};
			queueCreateInfos.push_back(queueCreateInfo);
		}
		VkPhysicalDeviceFeatures deviceFeatures = {
		};
		VkDeviceCreateInfo createInfo = {
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			nullptr,// pNext
			0,// flags
			static_cast<uint32_t>(queueCreateInfos.size()),
			queueCreateInfos.data(),
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
			static_cast<uint32_t>(requiredLayers.size()),
			requiredLayers.data(),
#else
			0,
			nullptr,
#endif
			static_cast<uint32_t>(requiredPhysicalDeviceExtensions.size()),
			requiredPhysicalDeviceExtensions.data(),
			&deviceFeatures
		};
		if (vkCreateDevice(retVal->physicalDevice, &createInfo, 
						   nullptr, &retVal->device) != VK_SUCCESS)
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to create Vulkan logical device!\n");
			delete retVal;
			return nullptr;
		}
		vkGetDeviceQueue(retVal->device, 
						 static_cast<uint32_t>(qfi.graphicsFamily),
						 0, &retVal->graphicsQueue);
		vkGetDeviceQueue(retVal->device, 
						 static_cast<uint32_t>(qfi.presentFamily), 
						 0, &retVal->presentQueue);
	}
	if (!retVal->createSwapChain())
	{
		delete retVal;
		return nullptr;
	}
	if(!retVal->createImageViews())
	{
		delete retVal;
		return nullptr;
	}
	if(!retVal->createRenderPass())
	{
		delete retVal;
		return nullptr;
	}
	if(!retVal->createFramebuffers())
	{
		delete retVal;
		return nullptr;
	}
	// Create Vulkan command pool //
	{
		QueueFamilyIndices queueFamilyIndices = 
			retVal->findQueueFamilies(retVal->physicalDevice);
		VkCommandPoolCreateInfo poolCreateInfo = {
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,// pNext
			///TODO: give memory allocation hint via flags for how often the
			///	command buffers need to be re-recorded
			0,// flags
			static_cast<uint32_t>(queueFamilyIndices.graphicsFamily)
		};
		if (vkCreateCommandPool(retVal->device,
								&poolCreateInfo,
								nullptr,
								&retVal->commandPool) != VK_SUCCESS)
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to create Vulkan command pool!\n");
			delete retVal;
			return nullptr;
		}
	}
	if(!retVal->createCommandBuffers())
	{
		delete retVal;
		return nullptr;
	}
	// create drawing synchronization tools //
	{
		const VkSemaphoreCreateInfo semaphoreCreateInfo = {
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			nullptr,// pNext
			0 // flags
		};
		const VkFenceCreateInfo fenceCreateInfo = {
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			nullptr,// pNext
			VK_FENCE_CREATE_SIGNALED_BIT // flags
		};
		retVal->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		retVal->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		retVal->frameFences.resize(MAX_FRAMES_IN_FLIGHT);
		for (size_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
		{
			if (vkCreateSemaphore(retVal->device,
								  &semaphoreCreateInfo,
								  nullptr,
								  &retVal->imageAvailableSemaphores[f]) != VK_SUCCESS)
			{
				SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
					"Failed to allocate Vulkan semaphore!\n");
				delete retVal;
				return nullptr;
			}
			if (vkCreateSemaphore(retVal->device,
								  &semaphoreCreateInfo,
								  nullptr,
								  &retVal->renderFinishedSemaphores[f]) != VK_SUCCESS)
			{
				SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
					"Failed to allocate Vulkan semaphore!\n");
				delete retVal;
				return nullptr;
			}
			if (vkCreateFence(retVal->device,
							  &fenceCreateInfo,
							  nullptr,
							  &retVal->frameFences[f]) != VK_SUCCESS)
			{
				SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
					"Failed to allocate Vulkan frame fence!\n");
				delete retVal;
				return nullptr;
			}
		}
	}
	// Create Vertex Buffer //
	{
		const vector<k10::Vertex> vertices = {
			{{-0.5f,  0.5f}, {1.f, 1.f, 0.f, 1.f}},
			{{-0.5f, -0.5f}, {1.f, 0.f, 0.f, 1.f}},
			{{ 0.5f,  0.5f}, {0.f, 0.f, 1.f, 1.f}},
			{{ 0.5f, -0.5f}, {0.f, 1.f, 0.f, 1.f}}
		};
		retVal->vertexBufferCount = static_cast<uint32_t>(vertices.size());
		const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		if (!retVal->vertexBuffer.createBuffer(retVal->device, 
											   retVal->physicalDevice,
											   bufferSize,
											   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
											   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
													VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to create vertexBuffer!\n");
			delete retVal;
			return nullptr;
		}
		void* data = nullptr;
		retVal->vertexBuffer.mapMemory(&data, 0, bufferSize);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		retVal->vertexBuffer.unmapMemory();
	}
	if (!retVal->quadPool.fillPool(retVal->device, retVal->physicalDevice,
								   retVal->commandPool,
								   static_cast<size_t>(1e6)))
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to fill quad pool!\n");
		delete retVal;
		return nullptr;
	}
	return retVal;
}
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
VKAPI_ATTR VkBool32 VKAPI_CALL k10::RenderWindow::vulkanDebugMessengerCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "validation layer ERROR='%s'\n",
			pCallbackData->pMessage);
		SDL_assert(false);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "validation layer WARNING='%s'\n",
			pCallbackData->pMessage);
		SDL_assert(false);
	}
#ifdef K10_VULKAN_DEBUG_VERBOSE
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
			"validation layer INFO='%s'\n", pCallbackData->pMessage);
	}
	else //if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	{
		SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
			"validation layer VERBOSE='%s'\n", pCallbackData->pMessage);
	}
#endif
	return VK_FALSE;
}
#endif
bool k10::RenderWindow::checkPhysicalDeviceExtensionSupport(
	VkPhysicalDevice pd, vector<char const*>const& requiredExtensionNames)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(pd, nullptr, &extensionCount, nullptr);
	vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(pd, nullptr, &extensionCount, availableExtensions.data());
	SDL_Log("physical device available extensions:\n");
	for (VkExtensionProperties const& ep : availableExtensions)
	{
		SDL_Log("\t'%s' (version %i)\n", ep.extensionName, ep.specVersion);
	}
	for (char const* const ren : requiredExtensionNames)
	{
		bool isAvailable = false;
		for (VkExtensionProperties const& ep : availableExtensions)
		{
			if (strcmp(ep.extensionName, ren) == 0)
			{
				isAvailable = true;
				break;
			}
		}
		if (!isAvailable)
		{
			return false;
		}
	}
	return true;
}
///void k10::RenderWindow::destroyRenderWindow(RenderWindow* rw)
///{
///	delete rw;
///}
k10::RenderWindow::~RenderWindow()
{
	quadPool.drainPool();
	vertexBuffer.destroyBuffer();
	cleanupSwapChain();
	for (size_t f = 0; f < MAX_FRAMES_IN_FLIGHT; f++)
	{
		vkDestroySemaphore(device, renderFinishedSemaphores[f], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[f], nullptr);
		vkDestroyFence(device, frameFences[f], nullptr);
	}
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDevice(device, nullptr);
#ifdef K10_ENABLE_VULKAN_VALIDATION_LAYERS
	PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessengerEXT;
	destroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (destroyDebugUtilsMessengerEXT)
	{
		destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	else
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, 
			"Failed to obtain vkDestroyDebugUtilsMessengerEXT!\n");
		SDL_assert(false);
	}
#endif
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
	SDL_DestroyWindow(window);
}
bool k10::RenderWindow::recordCommandBuffers(GfxPipelineIndex gpi)
{
	if (quadPool.flushRequired())
	{
		quadPool.flushVertexStaging(graphicsQueue);
	}
	gpiRecordedCommandBuffer = gpi;
	GfxPipeline const*const pipeline = findGfxPipeline(gpi);
	SDL_assert(pipeline);
	for (size_t c = 0; c < commandBuffers.size(); c++)
	{
		VkCommandBufferBeginInfo beginInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,// pNext
			///TODO: use flags for re-recorded command buffers
			0,// flags
			nullptr // inheritanceInfo
		};
		if (vkBeginCommandBuffer(commandBuffers[c], 
								 &beginInfo) != VK_SUCCESS)
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to begin recording Vulkan command buffer!\n");
			return false;
		}
		// render pass definition //
		{
			VkClearValue renderPassClearValue = { 0.f, 0.f, 0.f, 1.f };
			VkRenderPassBeginInfo renderPassInfo = {
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				nullptr,// pNext
				renderPass,
				swapChainFramebuffers[c],
				VkRect2D{ {0, 0}, swapChainExtent},
				1,// clear value count
				&renderPassClearValue
			};
			vkCmdBeginRenderPass(commandBuffers[c], 
								 &renderPassInfo, 
								 VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[c],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline->getPipeline());
			quadPool.issueCommands(commandBuffers[c]);
///			VkBuffer vertexBuffers[] = { vertexBuffer.getBuffer() };
///			VkDeviceSize vbOffsets[] = { 0 };
///			vkCmdBindVertexBuffers(commandBuffers[c], 0, 1, vertexBuffers, vbOffsets);
///			vkCmdDraw(commandBuffers[c], vertexBufferCount, 1, 0, 0);
			vkCmdEndRenderPass(commandBuffers[c]);
		}
		if (vkEndCommandBuffer(commandBuffers[c]) != VK_SUCCESS)
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed Vulkan command buffer recording failure!\n");
			return false;
		}
	}
	return true;
}
bool k10::RenderWindow::drawFrame()
{
	if (windowMinimized)
	{
		return true;
	}
	vkWaitForFences(device, 1, &frameFences[currentFrame], VK_TRUE, UINT64_MAX);
	if (quadPool.flushRequired())
	{
		vkFreeCommandBuffers(device, commandPool,
			static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		if (!createCommandBuffers())
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to re-create render pass command buffer!\n");
			return false;
		}
		if (!recordCommandBuffers(gpiRecordedCommandBuffer))
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to re-record render pass command buffer!\n");
			return false;
		}
	}
	// aquire the next image in the swapchain //
	uint32_t imageIndex;
	const VkResult resultAcquireNextImage =
		vkAcquireNextImageKHR(device, 
							  swapChain, 
							  UINT64_MAX, 
							  imageAvailableSemaphores[currentFrame],
							  VK_NULL_HANDLE, 
							  &imageIndex);
	if (resultAcquireNextImage == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return rebuildSwapChain();
	}
	else if (resultAcquireNextImage != VK_SUCCESS && 
			 resultAcquireNextImage != VK_SUBOPTIMAL_KHR)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed acquire next image!\n");
		return false;
	}
	// submit command buffers to operate on this image //
	VkSemaphore waitSemaphores[]   = { imageAvailableSemaphores[currentFrame] };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,// pNext
		1,// wait semaphore count
		waitSemaphores,
		waitStages,
		1,// command buffer count
		&commandBuffers[imageIndex],
		1,// signal semaphore count
		signalSemaphores
	};
	vkResetFences(device, 1, &frameFences[currentFrame]);
	const VkResult resultGfxSubmitQ =
		vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameFences[currentFrame]);
	if (resultGfxSubmitQ != VK_SUCCESS)
	{
		switch (resultGfxSubmitQ)
		{
		case VK_ERROR_DEVICE_LOST:
			///TODO: figure out why this randomly procs w/ large quad pool populations? ????
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to submit draw command buffer! (device lost)\n");
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to submit draw command buffer! (out of device memory)\n");
			break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to submit draw command buffer! (out of host memory)\n");
			break;
		default:
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to submit draw command buffer!\n");
			break;
		}
		return false;
	}
	// present the next image in the swap chain //
	VkPresentInfoKHR presentInfo = {
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,// pNext
		1,// wait semaphore count
		signalSemaphores,
		1,// swap chain count
		&swapChain,
		&imageIndex,
		nullptr // array of VkResults
	};
	const VkResult resultQueuePresent =
		vkQueuePresentKHR(presentQueue, &presentInfo);
	if (resultQueuePresent == VK_ERROR_OUT_OF_DATE_KHR || 
		resultQueuePresent == VK_SUBOPTIMAL_KHR ||
		windowResized)
	{
		if (!rebuildSwapChain())
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to rebuild swap chain!\n");
			return false;
		}
	}
	else if(resultQueuePresent != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to present swap chain image!\n");
		return false;
	}
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return true;
}
void k10::RenderWindow::waitForOperationsToFinish()
{
	vkDeviceWaitIdle(device);
}
void k10::RenderWindow::onWindowEvent(SDL_WindowEvent const& we)
{
	switch (we.event)
	{
	case SDL_WINDOWEVENT_SHOWN:
///		SDL_Log("window shown!\n");
		break;
	case SDL_WINDOWEVENT_HIDDEN:
///		SDL_Log("window hidden!\n");
		break;
	case SDL_WINDOWEVENT_EXPOSED:
///		SDL_Log("window exposed!\n");
		break;
	case SDL_WINDOWEVENT_MAXIMIZED:
///		SDL_Log("window maximized!\n");
		break;
	case SDL_WINDOWEVENT_RESTORED:
		windowMinimized = false;
		break;
	case SDL_WINDOWEVENT_RESIZED:
		windowResized = true;
		break;
	case SDL_WINDOWEVENT_MINIMIZED:
		windowMinimized = true;
		break;
	}
}
k10::QuadPool& k10::RenderWindow::getQuadPool()
{
	return quadPool;
}
k10::GfxPipelineIndex k10::RenderWindow::createGfxPipeline(
	GfxProgram const* vertProgram, GfxProgram const* fragProgram)
{
	if (gfxPipelines.size() >= MAX_PIPELINES)
	{
		return MAX_PIPELINES;
	}
	GfxPipeline newPipeline;
	vector<VkPipelineShaderStageCreateInfo> shaderStages = {
		vertProgram->getPipelineShaderStageCreateInfo(),
		fragProgram->getPipelineShaderStageCreateInfo() };
	if (!newPipeline.createPipeline(
			nextGpi, device, swapChainExtent, shaderStages, renderPass))
	{
		return MAX_PIPELINES;
	}
	do
	{
		nextGpi = (nextGpi + 1) % MAX_PIPELINES;
	} while (findGfxPipeline(nextGpi) != nullptr && 
			 gfxPipelines.size() < MAX_PIPELINES);
	gfxPipelines.push_back(newPipeline);
	return newPipeline.getGpi();
}
k10::GfxProgram* k10::RenderWindow::createGfxProgram(GfxProgram::ShaderType st)
{
	return new GfxProgram(device, st);
}
///VkDevice k10::RenderWindow::getDevice() const
///{
///	return device;
///}
void k10::RenderWindow::cleanupSwapChain()
{
	for (VkFramebuffer fb : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(device, fb, nullptr);
	}
	for (GfxPipeline p : gfxPipelines)
	{
		p.destroy(device);
	}
	vkFreeCommandBuffers(device, commandPool,
		static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	vkDestroyRenderPass(device, renderPass, nullptr);
	for (VkImageView iv : swapChainImageViews)
	{
		vkDestroyImageView(device, iv, nullptr);
	}
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}
bool k10::RenderWindow::rebuildSwapChain()
{
	windowResized = false;
	vkDeviceWaitIdle(device);
	cleanupSwapChain();
	if (!createSwapChain())
	{
		return false;
	}
	if (!createImageViews())
	{
		return false;
	}
	if (!createRenderPass())
	{
		return false;
	}
	if (!createFramebuffers())
	{
		return false;
	}
	if (!createCommandBuffers())
	{
		return false;
	}
	// rebuild gfx pipelines //
	{
		for (GfxPipeline& p : gfxPipelines)
		{
			if (!p.buildPipelineFromCache(device, swapChainExtent, renderPass))
			{
				return false;
			}
		}
	}
	// re-record command buffers //
	{
		if (!recordCommandBuffers(gpiRecordedCommandBuffer))
		{
			return false;
		}
	}
	return true;
}
bool k10::RenderWindow::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = 
		querySwapChainSupport(physicalDevice);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode     = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent                = chooseSwapExtent(swapChainSupport.capabilities);
	uint32_t imageCountMin = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCountMin > swapChainSupport.capabilities.maxImageCount)
	{
		imageCountMin = swapChainSupport.capabilities.maxImageCount;
	}
	QueueFamilyIndices queueFamilyIndices = 
		findQueueFamilies(physicalDevice);
	vector<uint32_t> qfiVec = queueFamilyIndices.toVkVector();
	VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	uint32_t queueFamilyIndexCount = 0;
	uint32_t const* pQueueFamilyIndices = nullptr;
	if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
	{
		imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		queueFamilyIndexCount = 2;
		pQueueFamilyIndices = qfiVec.data();
	}
	VkSwapchainCreateInfoKHR createInfo{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		nullptr,// pNext
		0,// flags
		surface,
		imageCountMin,
		surfaceFormat.format,
		surfaceFormat.colorSpace,
		extent,
		1,// imageArrayLayers, always 1 unless we're doing stereoscopic 3D
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		imageSharingMode,
		queueFamilyIndexCount,
		pQueueFamilyIndices,
		swapChainSupport.capabilities.currentTransform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		presentMode,
		VK_TRUE,// clipped means we don't care about the window's obscured pixels
		VK_NULL_HANDLE // old swap chain (only used when resizing window etc...)
	};
	if (vkCreateSwapchainKHR(device, &createInfo, 
							 nullptr, &swapChain) != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create Vulkan swapchain!\n");
		return false;
	}
	uint32_t swapChainImgCount;
	vkGetSwapchainImagesKHR(device, swapChain, 
							&swapChainImgCount, nullptr);
	swapChainImages.resize(swapChainImgCount);
	vkGetSwapchainImagesKHR(device, swapChain, 
							&swapChainImgCount, swapChainImages.data());
	swapChainFormat = surfaceFormat.format;
	swapChainExtent = extent;
	return true;
}
bool k10::RenderWindow::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		VkComponentMapping componentMapping = {
			VK_COMPONENT_SWIZZLE_IDENTITY,// r
			VK_COMPONENT_SWIZZLE_IDENTITY,// g
			VK_COMPONENT_SWIZZLE_IDENTITY,// b
			VK_COMPONENT_SWIZZLE_IDENTITY // a
		};
		VkImageSubresourceRange subresourceRange = {
			VK_IMAGE_ASPECT_COLOR_BIT,
			0,// baseMipLevel
			1,// levelCount
			0,// baseArrayLayer
			1 // layerCount
		};
		VkImageViewCreateInfo createInfo = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,// pNext
			0,// flags
			swapChainImages[i],
			VK_IMAGE_VIEW_TYPE_2D,
			swapChainFormat,
			componentMapping,
			subresourceRange
		};
		if (vkCreateImageView(device, &createInfo, nullptr,
							  &swapChainImageViews[i]) != VK_SUCCESS)
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to create Vulkan swapchain image view!\n");
			return false;
		}
	}
	return true;
}
bool k10::RenderWindow::createRenderPass()
{
	VkAttachmentDescription colorAttachment = {
		0,// flags
		swapChainFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,// stencil
		VK_ATTACHMENT_STORE_OP_DONT_CARE,// stencil
		VK_IMAGE_LAYOUT_UNDEFINED,// initial layout
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // final layout
	};
	VkAttachmentReference colorAttachmentRef = {
		0,// attachment
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkSubpassDescription subpass = {
		0,// flags
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,// inputAttachmentCount
		nullptr, // input attachments
		1,// color attachment count
		&colorAttachmentRef,
		nullptr,// resolve attachments
		nullptr,// depth stencil attachment
		0,// preserve attachment count
		nullptr // preserve attachments
	};
	VkSubpassDependency dependency = {
		VK_SUBPASS_EXTERNAL,// src subpass
		0,// dst subpass
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,// src stage mask
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,// dst stage mask
		0,// src access mask
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,// dst access mask
		0 // flags
	};
	VkRenderPassCreateInfo renderPassCreateInfo = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,// pNext
		0,// flags
		1,// attachment count
		&colorAttachment,
		1,// subpass count
		&subpass,
		1,// dependency count
		&dependency // dependencies
	};
	if(vkCreateRenderPass(device, 
						  &renderPassCreateInfo, 
						  nullptr,
						  &renderPass) != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to create Vulkan render pass!\n");
		return false;
	}
	return true;
}
bool k10::RenderWindow::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t f = 0; f < swapChainImageViews.size(); f++)
	{
		VkFramebufferCreateInfo createInfo = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,// pNext
			0,// flags
			renderPass,
			1,// attachment count
			&swapChainImageViews[f],
			swapChainExtent.width,
			swapChainExtent.height,
			1 // layers
		};
		if (vkCreateFramebuffer(device,
								&createInfo,
								nullptr,
								&swapChainFramebuffers[f]) != VK_SUCCESS)
		{
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
				"Failed to create Vulkan framebuffer!\n");
			return false;
		}
	}
	return true;
}
bool k10::RenderWindow::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo allocateInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,// pNext
		commandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		static_cast<uint32_t>(commandBuffers.size())
	};
	if (vkAllocateCommandBuffers(device,
								 &allocateInfo,
								 commandBuffers.data()) != VK_SUCCESS)
	{
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
			"Failed to allocate Vulkan command buffers!\n");
		return false;
	}
	return true;
}
k10::RenderWindow::QueueFamilyIndices k10::RenderWindow::findQueueFamilies(VkPhysicalDevice pd) const
{
	QueueFamilyIndices retVal;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pd, &queueFamilyCount, nullptr);
	vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(pd, &queueFamilyCount, queueFamilyProps.data());
	for (uint64_t qfIndex = 0; static_cast<size_t>(qfIndex) < queueFamilyProps.size(); qfIndex++)
	{
		VkQueueFamilyProperties const& qfp = queueFamilyProps[qfIndex];
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(pd,
			static_cast<uint32_t>(qfIndex), surface, &presentSupport);
		if (qfp.queueCount > 0 && presentSupport)
		{
			retVal.presentFamily = qfIndex;
		}
		if (qfp.queueCount > 0 && qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			retVal.graphicsFamily = qfIndex;
		}
		// TODO: change this somehow so that we prefer drawing & presentation to be on the same queue
		if (retVal.isSuitable())
		{
			break;
		}
	}
	return retVal;
}
k10::RenderWindow::SwapChainSupportDetails k10::RenderWindow::querySwapChainSupport(
	VkPhysicalDevice pd) const
{
	SwapChainSupportDetails retVal;
	// query capabilities //
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &retVal.capabilities);
	}
	// query formats //
	{
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &formatCount, nullptr);
		if (formatCount > 0)
		{
			retVal.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(pd, 
				surface, &formatCount, retVal.formats.data());
		}
	}
	// query present modes //
	{
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &presentModeCount, nullptr);
		if (presentModeCount > 0)
		{
			retVal.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(pd, 
				surface, &presentModeCount, retVal.presentModes.data());
		}
	}
	return retVal;
}
VkExtent2D k10::RenderWindow::chooseSwapExtent(
	VkSurfaceCapabilitiesKHR const& c) const
{
	if (c.currentExtent.width != 
			numeric_limits<decltype(c.currentExtent.width)>::max())
	{
///		if (c.currentExtent.width == 0)
///		{
///			SDL_Log("uh oh...\n");
///		}
		return c.currentExtent;
	}
	int w, h;
	SDL_Vulkan_GetDrawableSize(window, &w, &h);
	return VkExtent2D{ 
		clamp(static_cast<uint32_t>(w), 
			  c.minImageExtent.width, c.maxImageExtent.width),
		clamp(static_cast<uint32_t>(h),
			  c.minImageExtent.height, c.maxImageExtent.height) };
}
VkPresentModeKHR k10::RenderWindow::chooseSwapPresentMode(
	vector<VkPresentModeKHR>const& presentModes) const
{
	for (VkPresentModeKHR const& pm : presentModes)
	{
		// Check for triple buffering support if available //
		if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return pm;
		}
	}
	// otherwise just use double buffering v-sync, because it is guaranteed //
	return VK_PRESENT_MODE_FIFO_KHR;
}
VkSurfaceFormatKHR k10::RenderWindow::chooseSwapSurfaceFormat(
	vector<VkSurfaceFormatKHR>const& formats) const
{
	SDL_assert(!formats.empty());
	for (VkSurfaceFormatKHR const& f : formats)
	{
		if (f.format == VK_FORMAT_R8G8B8A8_UNORM &&
			f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return f;
		}
	}
	return formats[0];
}
k10::GfxPipeline* k10::RenderWindow::findGfxPipeline(GfxPipelineIndex gpi)
{
	for (GfxPipeline& gp : gfxPipelines)
	{
		if (gp.getGpi() == gpi)
		{
			return &gp;
		}
	}
	return nullptr;
}
uint64_t k10::RenderWindow::findMemoryType(
	uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties physicalMemProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalMemProps);
	for (uint32_t t = 0; t < physicalMemProps.memoryTypeCount; t++)
	{
		if ((typeFilter & (1 << t)) &&
			(physicalMemProps.memoryTypes[t].propertyFlags & properties) == properties)
		{
			return t;
		}
	}
	return numeric_limits<uint64_t>::max();
}
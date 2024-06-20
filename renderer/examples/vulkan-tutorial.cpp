#include <core/File.h>
#include <core/Log.h>
#include <core/MiMallocator.h>
#include <core/Path.h>

#include <volk.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

class HelloTriangleApplication
{
public:
	HelloTriangleApplication(core::Log* log, core::Allocator* allocator)
		: m_allocator(allocator),
		  m_log(log),
		  m_swapchainImages(allocator),
		  m_swapchainImageViews(allocator)
	{}

	core::HumanError run()
	{
		if (auto err = initWindow()) return err;
		if (auto err = initVulkan()) return err;
		if (auto err = mainLoop()) return err;
		if (auto err = cleanup()) return err;
		return {};
	}

private:
	core::HumanError initWindow()
	{
		if (glfwInit() == GLFW_FALSE)
			return core::errf(m_allocator, "glfwInit failed"_sv);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
		return {};
	}

	core::HumanError initVulkan()
	{
		if (auto result = volkInitialize(); result != VK_SUCCESS)
			return core::errf(m_allocator, "volkInitialize failed, ErrorCode({})"_sv, result);

		if (auto err = createInstance()) return err;

		if (auto err = setupDebugMessenger()) return err;

		if (auto err = createSurface()) return err;

		if (auto err = pickPhysicalDevice()) return err;

		if (auto err = createLogicalDevice()) return err;

		if (auto err = createSwapchain()) return err;

		if (auto err = createImageViews()) return err;

		if (auto err = createGraphicsPipeline()) return err;

		return {};
	}

	core::HumanError mainLoop()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
		}
		return {};
	}

	core::HumanError cleanup()
	{
		if (m_pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
		for (auto view: m_swapchainImageViews)
			vkDestroyImageView(m_logicalDevice, view, nullptr);
		if (m_swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);
		if (m_logicalDevice != VK_NULL_HANDLE) vkDestroyDevice(m_logicalDevice, nullptr);
		if (m_surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		if (m_debugMessenger != VK_NULL_HANDLE) vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		if (m_instance != VK_NULL_HANDLE) vkDestroyInstance(m_instance, nullptr);

		volkFinalize();

		glfwDestroyWindow(m_window);
		glfwTerminate();
		return {};
	}

	core::HumanError createInstance()
	{
		if (m_enableValidationLayers)
			if (auto err = checkValidationLayerSupport())
				return err;

		VkApplicationInfo applicationInfo {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Hello Triangle",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "No Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_3,
		};

		auto extensions = getRequiredExtensions();

		VkInstanceCreateInfo instanceInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &applicationInfo,
			.enabledExtensionCount = (uint32_t)extensions.count(),
			.ppEnabledExtensionNames = extensions.data(),
		};

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_enableValidationLayers)
		{
			instanceInfo.enabledLayerCount = sizeof(VALIDATION_LAYERS) / sizeof(*VALIDATION_LAYERS);
			instanceInfo.ppEnabledLayerNames = VALIDATION_LAYERS;

			populateDebugMessengerCreateInfo(debugCreateInfo);
			instanceInfo.pNext = &debugCreateInfo;
		}

		auto result = vkCreateInstance(&instanceInfo, nullptr, &m_instance);
		if (result != VK_SUCCESS)
			return core::errf(m_allocator, "vkCreateInstance failed, ErrorCode({})"_sv, result);

		volkLoadInstance(m_instance);

		return {};
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
														VkDebugUtilsMessageTypeFlagsEXT type,
														const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
														void* pUserData)
	{
		auto log = (core::Log*)pUserData;

		const char* typeName = "unknown";
		if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			typeName = "general";
		else if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			typeName = "validation";
		else if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			typeName = "performance";
		else if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
			typeName = "binding";

		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			log->trace("[{}] {}"_sv, typeName, pCallbackData->pMessage);
		}
		else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			log->info("[{}] {}"_sv, typeName, pCallbackData->pMessage);
		}
		else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			log->warn("[{}] {}"_sv, typeName, pCallbackData->pMessage);
		}
		else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			log->error("[{}] {}"_sv, typeName, pCallbackData->pMessage);
		}
		else
		{
			coreUnreachable();
		}

		return VK_FALSE;
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debugCallback,
			.pUserData = m_log,
		};
	}

	core::HumanError setupDebugMessenger()
	{
		if (m_enableValidationLayers == false)
			return {};

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		auto result = vkCreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger);
		if (result != VK_SUCCESS)
			return core::errf(m_allocator, "vkCreateDebugUtilsMessengerEXT failed, ErrorCode({})"_sv, result);
		return {};
	}

	core::HumanError createSurface()
	{
		auto result = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
		if (result != VK_SUCCESS)
			return core::errf(m_allocator, "glfwCreateWindowSurface failed, ErrorCode({})"_sv, result);
		return {};
	}

	core::HumanError pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		core::Array<VkPhysicalDevice> devices{m_allocator};
		devices.resize_fill(deviceCount, VK_NULL_HANDLE);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		for (auto device: devices)
		{
			auto familyProperties = listPhysicalDeviceFamilyProperties(device);

			coreAssert(familyProperties.count() < UINT32_MAX);
			uint32_t graphicsQueueFamily = UINT32_MAX;
			uint32_t presentQueueFamily = UINT32_MAX;
			for (size_t i = 0; i < familyProperties.count(); ++i)
			{
				if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					graphicsQueueFamily = (uint32_t)i;

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, (uint32_t)i, m_surface, &presentSupport);

				if (presentSupport)
					presentQueueFamily = (uint32_t)i;

				if (graphicsQueueFamily != UINT32_MAX && presentQueueFamily != UINT32_MAX)
					break;
			}

			if (graphicsQueueFamily == UINT32_MAX || presentQueueFamily == UINT32_MAX)
				continue;

			auto extensions = listPhysicalDeviceExtensions(device);
			for (auto requiredExtensionName: DEVICE_EXTENSIONS)
				if (findExtensionByName(extensions, core::StringView{requiredExtensionName}) == SIZE_MAX)
					continue;

			auto swapchainSupport = querySwapchainSupport(device);
			if (swapchainSupport.formats.count() == 0 || swapchainSupport.presentModes.count() == 0)
				continue;

			m_graphicsQueueFamily = graphicsQueueFamily;
			m_presentQueueFamily = presentQueueFamily;
			m_physicalDevice = device;
			break;
		}

		if (m_physicalDevice == VK_NULL_HANDLE)
			return core::errf(m_allocator, "failed to find suitable physical device"_sv);
		return {};
	}

	core::HumanError createLogicalDevice()
	{
		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo graphicsQueueCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = m_graphicsQueueFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority,
		};

		VkDeviceQueueCreateInfo presentQueueCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = m_presentQueueFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority,
		};

		VkDeviceQueueCreateInfo queues[] = {graphicsQueueCreateInfo, presentQueueCreateInfo};
		uint32_t queuesCount = 2;
		if (m_presentQueueFamily == m_graphicsQueueFamily)
			queuesCount = 1;

		VkPhysicalDeviceFeatures deviceFeatures{};
		VkDeviceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = queuesCount,
			.pQueueCreateInfos = queues,
			.enabledExtensionCount = sizeof(DEVICE_EXTENSIONS) / sizeof(*DEVICE_EXTENSIONS),
			.ppEnabledExtensionNames = DEVICE_EXTENSIONS,
			.pEnabledFeatures = &deviceFeatures,
		};

		if (m_enableValidationLayers)
		{
			createInfo.enabledLayerCount = sizeof(VALIDATION_LAYERS) / sizeof(*VALIDATION_LAYERS);
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
		}

		auto result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice);
		if (result != VK_SUCCESS)
			return core::errf(m_allocator, "vkCreateDevice failed, ErrorCode({})"_sv, result);

		vkGetDeviceQueue(m_logicalDevice, m_graphicsQueueFamily, 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_logicalDevice, m_presentQueueFamily, 0, &m_presentQueue);

		return {};
	}

	core::HumanError createSwapchain()
	{
		auto swapchainSupport = querySwapchainSupport(m_physicalDevice);
		auto surfaceFormat = chooseSwapchainSurfaceFormat(swapchainSupport.formats);
		auto presentMode = chooseSwapchainPresentMode(swapchainSupport.presentModes);
		auto extent = chooseSwapchainExtent(swapchainSupport.capabilities);

		uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
		if (swapchainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapchainSupport.capabilities.maxImageCount)
			imageCount = swapchainSupport.capabilities.maxImageCount;

		uint32_t queueFamilyIndices[] = {m_graphicsQueueFamily, m_presentQueueFamily};

		VkSwapchainCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = m_surface,
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.preTransform = swapchainSupport.capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		if (m_graphicsQueueFamily != m_presentQueueFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		auto result = vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapchain);
		if (result != VK_SUCCESS)
			return core::errf(m_allocator, "vkCreateSwapchainKHR failed, ErrorCode({})"_sv, result);

		vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &imageCount, nullptr);
		m_swapchainImages.resize_fill(imageCount, VK_NULL_HANDLE);
		vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain, &imageCount, m_swapchainImages.data());

		m_swapchainFormat = surfaceFormat.format;
		m_swapchainExtent = extent;
		return {};
	}

	core::HumanError createImageViews()
	{
		m_swapchainImageViews.resize_fill(m_swapchainImages.count(), VK_NULL_HANDLE);
		for (size_t i = 0; i < m_swapchainImages.count(); ++i)
		{
			VkImageViewCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = m_swapchainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_swapchainFormat,
				.components = {
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY,
				},
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1
				},
			};

			auto result = vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &m_swapchainImageViews[i]);
			if (result != VK_SUCCESS)
				return core::errf(m_allocator, "vkCreateImageView failed, ErrorCode({})"_sv, result);
		}
		return {};
	}

	core::HumanError createGraphicsPipeline()
	{
		auto vertShaderPath = core::Path::join(m_allocator, core::StringView{SHADERS_DIR}, "vert.spv"_sv);
		auto fragShaderPath = core::Path::join(m_allocator, core::StringView{SHADERS_DIR}, "frag.spv"_sv);

		auto vertShaderResult = core::File::content(m_allocator, vertShaderPath);
		if (vertShaderResult.isError())
			return vertShaderResult.releaseError();
		auto vertShader = vertShaderResult.releaseValue();

		auto fragShaderResult = core::File::content(m_allocator, fragShaderPath);
		if (fragShaderResult.isError())
			return fragShaderResult.releaseError();
		auto fragShader = fragShaderResult.releaseValue();

		auto vertShaderModuleResult = createShaderModule(core::Span<const std::byte>{vertShader});
		if (vertShaderModuleResult.isError())
			return vertShaderModuleResult.releaseError();
		auto vertShaderModule = vertShaderModuleResult.releaseValue();
		coreDefer { vkDestroyShaderModule(m_logicalDevice, vertShaderModule, nullptr); };

		auto fragShaderModuleResult = createShaderModule(core::Span<const std::byte>{fragShader});
		if (fragShaderModuleResult.isError())
			return fragShaderModuleResult.releaseError();
		auto fragShaderModule = fragShaderModuleResult.releaseValue();
		coreDefer { vkDestroyShaderModule(m_logicalDevice, fragShaderModule, nullptr); };

		VkPipelineShaderStageCreateInfo vertShaderStageInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertShaderModule,
			.pName = "main",
		};

		VkPipelineShaderStageCreateInfo fragShaderStageInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragShaderModule,
			.pName = "main",
		};

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssembly {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		};

		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};

		VkPipelineRasterizationStateCreateInfo rasterizer {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.lineWidth = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo multisampling {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
		};

		VkPipelineColorBlendAttachmentState colorBlendAttachment {
			.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
							  VK_COLOR_COMPONENT_G_BIT |
							  VK_COLOR_COMPONENT_B_BIT |
							  VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineColorBlendStateCreateInfo colorBlending {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
		};

		VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = sizeof(dynamicStates) / sizeof(*dynamicStates),
			.pDynamicStates = dynamicStates,
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		};

		auto result = vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
		if (result != VK_SUCCESS)
			return core::errf(m_allocator, "vkCreatePipelineLayout failed, ErrorCode({})"_sv, result);

		return {};
	}

	core::Result<VkShaderModule> createShaderModule(core::Span<const std::byte> code)
	{
		VkShaderModuleCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = code.sizeInBytes(),
			.pCode = (uint32_t*)code.data(),
		};

		VkShaderModule module = VK_NULL_HANDLE;
		auto result = vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &module);
		if (result != VK_SUCCESS)
			return core::errf(m_allocator, "vkCreateShaderModule failed, ErrorCode({})"_sv, result);
		return module;
	}

	VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const core::Array<VkSurfaceFormatKHR>& formats)
	{
		for (auto format: formats)
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;
		return formats[0];
	}

	VkPresentModeKHR chooseSwapchainPresentMode(const core::Array<VkPresentModeKHR>& modes)
	{
		for (auto mode: modes)
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
			return capabilities.currentExtent;

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actualExtent {
			.width = (uint32_t)width,
			.height = (uint32_t)height,
		};

		if (actualExtent.width < capabilities.minImageExtent.width)
			actualExtent.width = capabilities.minImageExtent.width;
		if (actualExtent.width > capabilities.maxImageExtent.width)
			actualExtent.width = capabilities.maxImageExtent.width;

		if (actualExtent.height < capabilities.minImageExtent.height)
			actualExtent.height = capabilities.minImageExtent.height;
		if (actualExtent.height > capabilities.maxImageExtent.height)
			actualExtent.height = capabilities.maxImageExtent.height;

		return actualExtent;
	}

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		core::Array<VkSurfaceFormatKHR> formats;
		core::Array<VkPresentModeKHR> presentModes;

		SwapchainSupportDetails(core::Allocator* allocator)
			: formats(allocator),
			  presentModes(allocator)
		{}
	};

	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device)
	{
		SwapchainSupportDetails result{m_allocator};

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &result.capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			result.formats.resize_fill(formatCount, VkSurfaceFormatKHR{});
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, result.formats.data());
		}

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			result.presentModes.resize_fill(presentModeCount, VkPresentModeKHR{});
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, result.presentModes.data());
		}

		return result;
	}

	size_t findExtensionByName(const core::Array<VkExtensionProperties>& extensions, core::StringView name)
	{
		for (size_t i = 0; i < extensions.count(); ++i)
			if (core::StringView{extensions[i].extensionName} == name)
				return i;
		return SIZE_MAX;
	}

	core::Array<VkExtensionProperties> listPhysicalDeviceExtensions(VkPhysicalDevice device)
	{
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		core::Array<VkExtensionProperties> result{m_allocator};
		result.resize_fill(extensionCount, VkExtensionProperties{});
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, result.data());

		return result;
	}

	core::Array<VkQueueFamilyProperties> listPhysicalDeviceFamilyProperties(VkPhysicalDevice device)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		core::Array<VkQueueFamilyProperties> result{m_allocator};
		result.resize_fill(queueFamilyCount, VkQueueFamilyProperties{});
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, result.data());

		return result;
	}

	core::Array<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		core::Array<const char*> extensions{m_allocator};
		for (uint32_t i = 0; i < glfwExtensionCount; ++i)
			extensions.push(glfwExtensions[i]);

		if (m_enableValidationLayers)
			extensions.push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		return extensions;
	}

	core::HumanError checkValidationLayerSupport()
	{
		auto layers = listInstanceLayers();
		for (auto requiredLayer: VALIDATION_LAYERS)
			if (findLayerByName(layers, core::StringView{requiredLayer}) == SIZE_MAX)
				return core::errf(m_allocator, "required layer '{}' not found"_sv, requiredLayer);
		return {};
	}

	core::Array<VkLayerProperties> listInstanceLayers()
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		core::Array<VkLayerProperties> result{m_allocator};
		result.resize_fill(layerCount, VkLayerProperties{});
		vkEnumerateInstanceLayerProperties(&layerCount, result.data());

		return result;
	}

	size_t findLayerByName(const core::Array<VkLayerProperties>& layers, core::StringView layerName)
	{
		for (size_t i = 0; i < layers.count(); ++i)
			if (core::StringView{layers[i].layerName} == layerName)
				return i;
		return SIZE_MAX;
	}

	core::Allocator* m_allocator = nullptr;
	core::Log* m_log = nullptr;
	GLFWwindow* m_window = nullptr;
	VkInstance m_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	uint32_t m_graphicsQueueFamily = UINT32_MAX;
	uint32_t m_presentQueueFamily = UINT32_MAX;
	VkDevice m_logicalDevice = VK_NULL_HANDLE;
	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue m_presentQueue = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkFormat m_swapchainFormat = {};
	VkExtent2D m_swapchainExtent = {};
	core::Array<VkImage> m_swapchainImages;
	core::Array<VkImageView> m_swapchainImageViews;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	bool m_enableValidationLayers = true;
	constexpr static const char* VALIDATION_LAYERS[] = {
		"VK_LAYER_KHRONOS_validation",
	};
	constexpr static const char* DEVICE_EXTENSIONS[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	constexpr static uint32_t WINDOW_WIDTH = 800;
	constexpr static uint32_t WINDOW_HEIGHT = 600;
};

int main()
{
	core::Mimallocator allocator{};
	core::Log log{&allocator};

	HelloTriangleApplication app{&log, &allocator};
	auto err = app.run();
	if (err)
	{
		log.critical("{}"_sv, err);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

namespace fmt
{
	template <> struct formatter<VkResult>
	{
		template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext> auto format(VkResult r, FormatContext& ctx)
		{
			switch (r)
			{
			case VK_SUCCESS:
				return fmt::format_to(ctx.out(), "VK_SUCCESS");
			case VK_NOT_READY:
				return fmt::format_to(ctx.out(), "VK_NOT_READY");
			case VK_TIMEOUT:
				return fmt::format_to(ctx.out(), "VK_TIMEOUT");
			case VK_EVENT_SET:
				return fmt::format_to(ctx.out(), "VK_EVENT_SET");
			case VK_EVENT_RESET:
				return fmt::format_to(ctx.out(), "VK_EVENT_RESET");
			case VK_INCOMPLETE:
				return fmt::format_to(ctx.out(), "VK_INCOMPLETE");
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				return fmt::format_to(ctx.out(), "VK_ERROR_OUT_OF_HOST_MEMORY");
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				return fmt::format_to(ctx.out(), "VK_ERROR_OUT_OF_DEVICE_MEMORY");
			case VK_ERROR_INITIALIZATION_FAILED:
				return fmt::format_to(ctx.out(), "VK_ERROR_INITIALIZATION_FAILED");
			case VK_ERROR_DEVICE_LOST:
				return fmt::format_to(ctx.out(), "VK_ERROR_DEVICE_LOST");
			case VK_ERROR_MEMORY_MAP_FAILED:
				return fmt::format_to(ctx.out(), "VK_ERROR_MEMORY_MAP_FAILED");
			case VK_ERROR_LAYER_NOT_PRESENT:
				return fmt::format_to(ctx.out(), "VK_ERROR_LAYER_NOT_PRESENT");
			case VK_ERROR_EXTENSION_NOT_PRESENT:
				return fmt::format_to(ctx.out(), "VK_ERROR_EXTENSION_NOT_PRESENT");
			case VK_ERROR_FEATURE_NOT_PRESENT:
				return fmt::format_to(ctx.out(), "VK_ERROR_FEATURE_NOT_PRESENT");
			case VK_ERROR_INCOMPATIBLE_DRIVER:
				return fmt::format_to(ctx.out(), "VK_ERROR_INCOMPATIBLE_DRIVER");
			case VK_ERROR_TOO_MANY_OBJECTS:
				return fmt::format_to(ctx.out(), "VK_ERROR_TOO_MANY_OBJECTS");
			case VK_ERROR_FORMAT_NOT_SUPPORTED:
				return fmt::format_to(ctx.out(), "VK_ERROR_FORMAT_NOT_SUPPORTED");
			case VK_ERROR_FRAGMENTED_POOL:
				return fmt::format_to(ctx.out(), "VK_ERROR_FRAGMENTED_POOL");
			case VK_ERROR_UNKNOWN:
			default:
				return fmt::format_to(ctx.out(), "VK_ERROR_UNKNOWN");
			}
		}
	};
}
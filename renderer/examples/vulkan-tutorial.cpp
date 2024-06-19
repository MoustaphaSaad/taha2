#include <core/File.h>
#include <core/Log.h>
#include <core/MiMallocator.h>

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
		  m_log(log)
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
			for (size_t i = 0; i < familyProperties.count(); ++i)
			{
				if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					m_graphicsQueueFamily = (uint32_t)i;

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, (uint32_t)i, m_surface, &presentSupport);

				if (presentSupport)
					m_presentQueueFamily = (uint32_t)i;

				if (m_graphicsQueueFamily != UINT32_MAX && m_presentQueueFamily != UINT32_MAX)
					break;
			}

			if (m_graphicsQueueFamily != UINT32_MAX && m_presentQueueFamily != UINT32_MAX)
			{
				m_physicalDevice = device;
				break;
			}
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
	bool m_enableValidationLayers = true;
	constexpr static const char* VALIDATION_LAYERS[] = {
		"VK_LAYER_KHRONOS_validation",
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
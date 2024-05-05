#include "taha/vk/VkRenderer.h"
#include "taha/vk/VkFrame.h"

#include <core/Array.h>

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

namespace taha
{
#if TAHA_OS_WINDOWS
	LRESULT CALLBACK dummyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		return 0;
	}
#endif

	VkBool32 VKAPI_CALL VkRenderer::debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData)
	{
		auto renderer = (VkRenderer*)userData;
		auto log = renderer->m_log;

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
			log->trace("[{}] {}"_sv, typeName, callbackData->pMessage);
		}
		else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			log->info("[{}] {}"_sv, typeName, callbackData->pMessage);
		}
		else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			log->warn("[{}] {}"_sv, typeName, callbackData->pMessage);
		}
		else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			log->error("[{}] {}"_sv, typeName, callbackData->pMessage);
		}
		else
		{
			coreUnreachable();
		}

		return VK_FALSE;
	}

	constexpr const char* REQUIRED_DEVICE_EXTENSIONS[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	static bool checkPhysicalDeviceExtensions(VkPhysicalDevice device, core::Allocator* allocator)
	{
		uint32_t extensionCount = 0;
		auto res = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		if (res != VK_SUCCESS)
			return false;

		core::Array<VkExtensionProperties> availableExtensions{allocator};
		availableExtensions.resize_fill(extensionCount, VkExtensionProperties{});
		res = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.begin());
		if (res != VK_SUCCESS)
			return false;

		int requiredExtensionsCount = sizeof(REQUIRED_DEVICE_EXTENSIONS) / sizeof(*REQUIRED_DEVICE_EXTENSIONS);
		for (auto extension: availableExtensions)
		{
			for (auto requiredExtension: REQUIRED_DEVICE_EXTENSIONS)
			{
				if (core::StringView{extension.extensionName} == core::StringView{requiredExtension})
				{
					--requiredExtensionsCount;
					break;
				}
			}
		}

		return requiredExtensionsCount == 0;
	}

	struct PhysicalDeviceCheckResult
	{
		int graphicsFamilyIndex = -1;
		int presentationFamilyIndex = -1;
		bool requiredExtensionsSupported = false;

		bool suitable() const
		{
			return graphicsFamilyIndex != -1 && presentationFamilyIndex != -1 && requiredExtensionsSupported;
		}
	};

	static PhysicalDeviceCheckResult
	checkPhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR dummySurface, core::Allocator* allocator)
	{
		PhysicalDeviceCheckResult res{};
		res.requiredExtensionsSupported = checkPhysicalDeviceExtensions(device, allocator);

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		core::Array<VkQueueFamilyProperties> queueFamilies{allocator};
		queueFamilies.resize_fill(queueFamilyCount, VkQueueFamilyProperties{});
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.begin());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				res.graphicsFamilyIndex = int(i);

			VkBool32 presentationSupported = false;
			auto result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, dummySurface, &presentationSupported);
			if (result != VK_SUCCESS)
				continue;

			if (presentationSupported)
				res.presentationFamilyIndex = int(i);

			if (res.suitable())
				break;
		}

		return res;
	}

	core::Result<core::Unique<VkRenderer>> VkRenderer::create(core::Log* log, core::Allocator* allocator)
	{
		auto renderer = unique_from<VkRenderer>(allocator, log, allocator);

		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "No App",
			.applicationVersion = VK_MAKE_VERSION(0, 0, 0),
			.pEngineName = "Taha Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_0,
		};

		// layers
		const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

		uint32_t availableLayersCount = 0;
		auto result = vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkEnumerateInstanceLayerProperties failed, ErrorCode({})"_sv, result);

		core::Array<VkLayerProperties> availableLayers{allocator};
		availableLayers.resize_fill(availableLayersCount, VkLayerProperties{});
		result = vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.begin());
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkEnumerateInstanceLayerProperties failed, ErrorCode({})"_sv, result);

		for (auto requiredLayer: validationLayers)
		{
			bool found = false;
			for (auto availableLayer: availableLayers)
			{
				if (core::StringView{availableLayer.layerName} == core::StringView{requiredLayer})
				{
					found = true;
					break;
				}
			}
			if (found == false)
				return core::errf(allocator, "failed to find layer '{}'"_sv, requiredLayer);
		}

// extensions
#if TAHA_OS_WINDOWS
		const char* extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
#endif

		uint32_t availableExtensionsCount = 0;
		result = vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkEnumerateInstanceExtensionProperties failed, ErrorCode({})"_sv, result);

		core::Array<VkExtensionProperties> availableExtensions{allocator};
		availableExtensions.resize_fill(availableExtensionsCount, VkExtensionProperties{});
		result =
			vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.begin());
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkEnumerateInstanceExtensionProperties failed, ErrorCode({})"_sv, result);

		for (auto requiredExtension: extensions)
		{
			bool found = false;
			for (auto availableExtension: availableExtensions)
			{
				if (core::StringView{availableExtension.extensionName} == core::StringView{requiredExtension})
				{
					found = true;
					break;
				}
			}
			if (found == false)
				return core::errf(allocator, "failed to find extension '{}'"_sv, requiredExtension);
		}

		// debug messenger
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = VkRenderer::debugCallback,
			.pUserData = renderer.get()};

		// create instance
		VkInstanceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = &debugMessengerCreateInfo,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = sizeof(validationLayers) / sizeof(*validationLayers),
			.ppEnabledLayerNames = validationLayers,
			.enabledExtensionCount = sizeof(extensions) / sizeof(*extensions),
			.ppEnabledExtensionNames = extensions,
		};

		result = vkCreateInstance(&createInfo, nullptr, &renderer->m_instance);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkCreateInstance failed, ErrorCode({})"_sv, result);

		// create debug messenger
		auto createDebugUtilsMessengerEXTFn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			renderer->m_instance, "vkCreateDebugUtilsMessengerEXT");
		if (createDebugUtilsMessengerEXTFn == nullptr)
			return core::errf(
				allocator, "vkGetInstanceProcAddr failed, failed to find 'vkCreateDebugUtilsMessengerEXT'"_sv);
		renderer->m_destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			renderer->m_instance, "vkDestroyDebugUtilsMessengerEXT");
		if (renderer->m_destroyDebugMessengerFn == nullptr)
			return core::errf(
				allocator, "vkGetInstanceProcAddr failed, failed to find 'vkDestroyDebugUtilsMessengerEXT'"_sv);
		result = createDebugUtilsMessengerEXTFn(
			renderer->m_instance, &debugMessengerCreateInfo, nullptr, &renderer->m_debugMessenger);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkCreateDebugUtilsMessengerEXT failed, ErrorCode({})"_sv, result);

#if TAHA_OS_WINDOWS
		constexpr TCHAR CLASS_NAME[] = L"DummyWindowClass";
		WNDCLASSEX windowClass{};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpfnWndProc = dummyWindowProc;
		windowClass.hInstance = GetModuleHandle(nullptr);
		windowClass.lpszClassName = CLASS_NAME;
		RegisterClassEx(&windowClass);

		auto dummyHwnd = CreateWindowEx(
			0,
			CLASS_NAME,
			L"Dummy Window",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			nullptr,
			nullptr,
			GetModuleHandle(nullptr),
			nullptr);
		if (dummyHwnd == nullptr)
			return core::errf(allocator, "CreateWindowEx failed, ErrorCode({})"_sv, GetLastError());

		VkWin32SurfaceCreateInfoKHR dummySurfaceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = dummyHwnd,
		};

		VkSurfaceKHR dummySurface;
		result = vkCreateWin32SurfaceKHR(renderer->m_instance, &dummySurfaceCreateInfo, nullptr, &dummySurface);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkCreateWin32SurfaceKHR failed, ErrorCode({})"_sv, result);
#endif

		// choose physical device
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(renderer->m_instance, &deviceCount, nullptr);
		if (deviceCount == 0)
			return core::errf(allocator, "Failed to find GPUs with Vulkan support"_sv);
		core::Array<VkPhysicalDevice> physicalDevices{allocator};
		physicalDevices.resize_fill(deviceCount, {});
		vkEnumeratePhysicalDevices(renderer->m_instance, &deviceCount, physicalDevices.begin());

		PhysicalDeviceCheckResult physicalDeviceCheckResult{};
		for (auto device: physicalDevices)
		{
			physicalDeviceCheckResult = checkPhysicalDevice(device, dummySurface, allocator);
			if (physicalDeviceCheckResult.suitable())
			{
				renderer->m_physicalDevice = device;
				break;
			}
		}

		if (renderer->m_physicalDevice == VK_NULL_HANDLE)
			return core::errf(allocator, "cannot find suitable physical device"_sv);

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(renderer->m_physicalDevice, &deviceProperties);
		log->info(
			"name: {}, driver version: {}.{}.{}"_sv,
			deviceProperties.deviceName,
			VK_VERSION_MAJOR(deviceProperties.driverVersion),
			VK_VERSION_MINOR(deviceProperties.driverVersion),
			VK_VERSION_PATCH(deviceProperties.driverVersion));

		// create device
		int uniqueQueues[2] = {physicalDeviceCheckResult.graphicsFamilyIndex, -1};
		size_t uniqueQueuesCount = 1;
		if (physicalDeviceCheckResult.presentationFamilyIndex != physicalDeviceCheckResult.graphicsFamilyIndex)
		{
			uniqueQueues[1] = physicalDeviceCheckResult.presentationFamilyIndex;
			++uniqueQueuesCount;
		}

		float queuePriority = 1.0f;
		core::Array<VkDeviceQueueCreateInfo> queueCreateInfos{allocator};
		for (size_t i = 0; i < uniqueQueuesCount; ++i)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = uint32_t(uniqueQueues[i]),
				.queueCount = 1,
				.pQueuePriorities = &queuePriority,
			};
			queueCreateInfos.push(queueCreateInfo);
		}

		VkDeviceCreateInfo deviceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = uint32_t(queueCreateInfos.count()),
			.pQueueCreateInfos = queueCreateInfos.begin(),
			.enabledExtensionCount = sizeof(REQUIRED_DEVICE_EXTENSIONS) / sizeof(*REQUIRED_DEVICE_EXTENSIONS),
			.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS,
		};

		result = vkCreateDevice(renderer->m_physicalDevice, &deviceCreateInfo, nullptr, &renderer->m_device);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkCreateDevice failed, ErrorCode({})"_sv, result);

		vkGetDeviceQueue(
			renderer->m_device, physicalDeviceCheckResult.graphicsFamilyIndex, 0, &renderer->m_graphicsQueue);

		vkGetDeviceQueue(
			renderer->m_device, physicalDeviceCheckResult.presentationFamilyIndex, 0, &renderer->m_presentQueue);

		return renderer;
	}

	VkRenderer::~VkRenderer()
	{
		if (m_device != VK_NULL_HANDLE)
			vkDestroyDevice(m_device, nullptr);

		if (m_debugMessenger != VK_NULL_HANDLE)
			m_destroyDebugMessengerFn(m_instance, m_debugMessenger, nullptr);

		if (m_instance != VK_NULL_HANDLE)
			vkDestroyInstance(m_instance, nullptr);
	}

	core::Unique<Frame> VkRenderer::createFrameForWindow(NativeWindowDesc desc)
	{
#if TAHA_OS_WINDOWS
		VkWin32SurfaceCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = desc.windowHandle,
		};

		VkSurfaceKHR surface;
		auto result = vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &surface);
		if (result != VK_SUCCESS)
			return nullptr;

		return core::unique_from<VkFrame>(m_allocator, this, surface);
#endif

		return nullptr;
	}

	void VkRenderer::submitCommandsAndExecute(Frame* frame, const core::Array<core::Unique<Command>>& commands)
	{
		// do nothing
	}
}
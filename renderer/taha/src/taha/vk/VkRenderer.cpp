#include "taha/vk/VkRenderer.h"
#include "taha/vk/VkFrame.h"

#include <core/Array.h>
#include <core/CUtils.h>

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
#elif TAHA_OS_LINUX
	static void registryListener(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
	{
		if (core::StringView{interface} == core::StringView{wl_compositor_interface.name})
		{
			*((wl_compositor**)data) = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
		}
	}

	static void registryRemover(void* data, wl_registry* registry, uint32_t id)
	{
		// do nothing
	}
#endif

	static core::Array<VkExtensionProperties> enumerateInstanceExtensions(core::Allocator* allocator)
	{
		uint32_t instanceExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
		core::Array<VkExtensionProperties> res{allocator};
		res.resize_fill(instanceExtensionCount, VkExtensionProperties{});
		vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, res.data());
		return res;
	}

	static size_t findExtensionByName(const core::Array<VkExtensionProperties>& availableExtensions, core::StringView requiredExtension)
	{
		for (size_t i = 0; i < availableExtensions.count(); ++i)
			if (core::StringView{availableExtensions[i].extensionName} == requiredExtension)
				return i;
		return SIZE_MAX;
	}

	static core::Array<VkLayerProperties> enumerateInstanceLayers(core::Allocator* allocator)
	{
		uint32_t instanceLayerCount = 0;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		core::Array<VkLayerProperties> res{allocator};
		res.resize_fill(instanceLayerCount, VkLayerProperties{});
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, res.data());
		return res;
	}

	static size_t findLayerByName(const core::Array<VkLayerProperties>& availableLayers, core::StringView requiredLayer)
	{
		for (size_t i = 0; i < availableLayers.count(); ++i)
			if (core::StringView{availableLayers[i].layerName} == requiredLayer)
				return i;
		return SIZE_MAX;
	}

	static int ratePhysicalDevice(VkPhysicalDevice physicalDevice)
	{
		int score = 0;

		VkPhysicalDeviceProperties physicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkPhysicalDeviceFeatures physicalDeviceFeatures{};
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

		if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;

		score += physicalDeviceProperties.limits.maxImageDimension2D;

		return score;
	}

	static core::Array<VkQueueFamilyProperties> enumerateQueueFamilies(VkPhysicalDevice device, core::Allocator* allocator)
	{
		uint32_t queueFamiliesCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, nullptr);

		core::Array<VkQueueFamilyProperties> res{allocator};
		res.resize_fill(queueFamiliesCount, VkQueueFamilyProperties{});
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, res.data());
		return res;
	}

	struct RequiredQueueFamilies
	{
		uint32_t graphicsFamily;
		uint32_t presentFamily;
	};

	static core::Result<RequiredQueueFamilies> getRequiredQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, core::Allocator* allocator)
	{
		bool graphicsFamilySet = false;
		bool presentFamilySet = false;

		RequiredQueueFamilies res{};
		auto families = enumerateQueueFamilies(device, allocator);
		for (size_t i = 0; i < families.count(); ++i)
		{
			auto& family = families[i];
			if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				res.graphicsFamily = (uint32_t)i;
				graphicsFamilySet = true;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, (uint32_t)i, surface, &presentSupport);

			if (presentSupport)
			{
				res.presentFamily = (uint32_t)i;
				presentFamilySet = true;
			}

			if (graphicsFamilySet && presentFamilySet)
				return res;
		}

		if (graphicsFamilySet == false)
			return core::errf(allocator, "physical device not suitable, no graphics family support"_sv);
		else if (presentFamilySet == false)
			return core::errf(allocator, "physical device not suitable, no present family support"_sv);
		else
			return res;
	}

	static core::Array<VkExtensionProperties> enumeratePhysicalDeviceExtensions(VkPhysicalDevice device, core::Allocator* allocator)
	{
		uint32_t physicalDeviceExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &physicalDeviceExtensionCount, nullptr);
		core::Array<VkExtensionProperties> res{allocator};
		res.resize_fill(physicalDeviceExtensionCount, VkExtensionProperties{});
		vkEnumerateDeviceExtensionProperties(device, nullptr, &physicalDeviceExtensionCount, res.data());
		return res;
	}

	struct SwapchainSupport2
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		core::Array<VkSurfaceFormatKHR> formats;
		core::Array<VkPresentModeKHR> presentModes;

		explicit SwapchainSupport2(core::Allocator* allocator)
			: formats(allocator),
			  presentModes(allocator)
		{}

		static SwapchainSupport2 query(VkPhysicalDevice device, VkSurfaceKHR surface, core::Allocator* allocator)
		{
			SwapchainSupport2 res{allocator};

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &res.capabilities);

			uint32_t formatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

			if (formatCount != 0)
			{
				res.formats.resize_fill(formatCount, VkSurfaceFormatKHR{});
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, res.formats.data());
			}

			uint32_t presentModeCount = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				res.presentModes.resize_fill(presentModeCount, VkPresentModeKHR{});
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, res.presentModes.data());
			}
			return res;
		}
	};

	static VkSurfaceFormatKHR chooseSwapchainFormat(const core::Array<VkSurfaceFormatKHR>& formats)
	{
		for (auto format: formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;
		}
		coreAssert(formats.count() > 0);
		return formats[0];
	}

	static VkPresentModeKHR choosePresentMode(const core::Array<VkPresentModeKHR>& presentModes)
	{
		for (auto mode: presentModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkBool32 VKAPI_CALL VkRenderer::debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData)
	{
		auto log = (core::Log*)userData;

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

	struct SwapchainSupport
	{
		VkSurfaceCapabilitiesKHR capabilities = {};
		core::Array<VkSurfaceFormatKHR> formats;
		core::Array<VkPresentModeKHR> presentModes;

		SwapchainSupport(core::Allocator* allocator): formats(allocator), presentModes(allocator) {}
	};

	static core::Result<SwapchainSupport>
	querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR dummySurface, core::Allocator* allocator)
	{
		SwapchainSupport support{allocator};
		auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, dummySurface, &support.capabilities);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed, ErrorCode({})"_sv, result);

		uint32_t formatCount{};
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, dummySurface, &formatCount, nullptr);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkGetPhysicalDeviceSurfaceFormatsKHR failed, ErrorCode({})"_sv, result);

		support.formats.resize_fill(formatCount, VkSurfaceFormatKHR{});
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, dummySurface, &formatCount, support.formats.data());
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkGetPhysicalDeviceSurfaceFormatsKHR failed, ErrorCode({})"_sv, result);

		uint32_t presentModeCount{};
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, dummySurface, &presentModeCount, nullptr);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkGetPhysicalDeviceSurfacePresentModesKHR failed, ErrorCode({})"_sv, result);

		support.presentModes.resize_fill(presentModeCount, VkPresentModeKHR{});
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(
			device, dummySurface, &presentModeCount, support.presentModes.data());
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkGetPhysicalDeviceSurfacePresentModesKHR failed, ErrorCode({})"_sv, result);

		return support;
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
		res = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
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
		bool swapchainSupported = false;

		bool suitable() const
		{
			return (
				graphicsFamilyIndex != -1 && presentationFamilyIndex != -1 && requiredExtensionsSupported &&
				swapchainSupported);
		}
	};

	static PhysicalDeviceCheckResult
	checkPhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR dummySurface, core::Allocator* allocator)
	{
		auto swapchainSupportResult = querySwapchainSupport(device, dummySurface, allocator);
		if (swapchainSupportResult.isError())
			return PhysicalDeviceCheckResult{};
		auto swapchainSupport = swapchainSupportResult.releaseValue();

		PhysicalDeviceCheckResult res{};
		res.requiredExtensionsSupported = checkPhysicalDeviceExtensions(device, allocator);
		res.swapchainSupported = swapchainSupport.formats.count() > 0 && swapchainSupport.presentModes.count() > 0;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		core::Array<VkQueueFamilyProperties> queueFamilies{allocator};
		queueFamilies.resize_fill(queueFamilyCount, VkQueueFamilyProperties{});
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

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
		// TODO: parametrize this
		constexpr bool enableValidation = true;
		const char* validationLayers2[] = {
			"VK_LAYER_KHRONOS_validation",
		};

		// init volk
		{
			static auto volkInitializeResult = volkInitialize();
			if (volkInitializeResult != VK_SUCCESS)
				return core::errf(allocator, "volkInitialize failed, ErrorCode({})"_sv, volkInitializeResult);
		}

		// create instance
		VkInstance instance2 = VK_NULL_HANDLE;
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo2{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
							   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debugCallback,
			.pUserData = log,
		};
		coreDefer
		{
			if (instance2 != VK_NULL_HANDLE) vkDestroyInstance(instance2, nullptr);
		};
		{
			VkApplicationInfo appInfo{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pEngineName = "Taha",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				.apiVersion = VK_API_VERSION_1_3,
			};

			core::Array<const char*> instanceExtensions{allocator};
			instanceExtensions.push(VK_KHR_SURFACE_EXTENSION_NAME);
#if TAHA_OS_WINDOWS
			instanceExtensions.push(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif TAHA_OS_LINUX
			instanceExtensions.push(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif

			if (enableValidation)
				instanceExtensions.push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			// check for instance extension availability
			auto availableExtensions = enumerateInstanceExtensions(allocator);
			for (auto requiredExtension: instanceExtensions)
				if (findExtensionByName(availableExtensions, core::StringView{requiredExtension}) == SIZE_MAX)
					return core::errf(allocator, "required extension '{}' is not supported"_sv, requiredExtension);

			// create instance
			VkInstanceCreateInfo instanceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pApplicationInfo = &appInfo,
				.enabledExtensionCount = (uint32_t)instanceExtensions.count(),
				.ppEnabledExtensionNames = instanceExtensions.data(),
			};

			if (enableValidation)
			{
				// check for instance layers availability
				auto availableLayers = enumerateInstanceLayers(allocator);
				for (auto requiredLayer: validationLayers2)
					if (findLayerByName(availableLayers, core::StringView{requiredLayer}) == SIZE_MAX)
						return core::errf(allocator, "required layer '{}' is not supported"_sv, requiredLayer);

				instanceCreateInfo.enabledLayerCount = sizeof(validationLayers2) / sizeof(*validationLayers2);
				instanceCreateInfo.ppEnabledLayerNames = validationLayers2;
				instanceCreateInfo.pNext = &debugMessengerCreateInfo2;
			}

			auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance2);
			if (result != VK_SUCCESS)
				return core::errf(allocator, "vkCreateInstance failed, ErrorCode({})"_sv, result);

			volkLoadInstance(instance2);
		}

		// create debug messenger
		VkDebugUtilsMessengerEXT debugMessenger2 = VK_NULL_HANDLE;
		coreDefer
		{
			if (debugMessenger2 != VK_NULL_HANDLE)
				vkDestroyDebugUtilsMessengerEXT(instance2, debugMessenger2, nullptr);
		};
		if (enableValidation)
		{
			auto result = vkCreateDebugUtilsMessengerEXT(instance2, &debugMessengerCreateInfo2, nullptr, &debugMessenger2);
			if (result != VK_SUCCESS)
				return core::errf(allocator, "vkCreateDebugUtilsMessengerEXT failed, ErrorCode({})"_sv, result);
		}

		// create dummy window to check for presentation support
		VkSurfaceKHR dummySurface2 = VK_NULL_HANDLE;
		coreDefer { if (dummySurface2 != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance2, dummySurface2, nullptr); };
		{
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
			coreDefer { CloseWindow(dummyHwnd); };

			VkWin32SurfaceCreateInfoKHR dummySurfaceCreateInfo{
				.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
				.hinstance = GetModuleHandle(nullptr),
				.hwnd = dummyHwnd,
			};

			auto result = vkCreateWin32SurfaceKHR(instance2, &dummySurfaceCreateInfo, nullptr, &dummySurface2);
			if (result != VK_SUCCESS)
				return core::errf(allocator, "vkCreateWin32SurfaceKHR failed, ErrorCode({})"_sv, result);
#elif TAHA_OS_LINUX
			auto display = wl_display_connect(nullptr);
			coreDefer{wl_display_disconnect(display);};

			auto registry = wl_display_get_registry(display);
			wl_compositor* compositor = nullptr;
			wl_registry_listener listener{
				.global = registryListener,
				.global_remove = registryRemover,
			};
			wl_registry_add_listener(registry, &listener, &compositor);
			coreDefer {wl_compositor_destroy(compositor);};
			wl_display_roundtrip(display);

			auto surface = wl_compositor_create_surface(compositor);
			coreDefer {wl_surface_destroy(surface);};

			VkWaylandSurfaceCreateInfoKHR dummySurfaceCreateInfo {
				.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
				.display = display,
				.surface = surface,
			};
			result = vkCreateWaylandSurfaceKHR(renderer->m_instance, &dummySurfaceCreateInfo, nullptr, &dummySurface2);
			if (result != VK_SUCCESS)
				return core::errf(allocator, "vkCreateWaylandSurfaceKHR failed, ErrorCode({})"_sv, result);
#endif
		}

		// create physical device
		VkPhysicalDevice physicalDevice2 = VK_NULL_HANDLE;
		RequiredQueueFamilies requiredQueueFamilies2{};
		constexpr const char* requiredDeviceExtensions[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};
		VkSurfaceFormatKHR swapchainSurfaceFormat{};
		VkPresentModeKHR swapchainPresentMode{};
		{
			uint32_t physicalDeviceCount = 0;
			auto result = vkEnumeratePhysicalDevices(instance2, &physicalDeviceCount, nullptr);
			if (result != VK_SUCCESS)
				return core::errf(allocator, "vkEnumeratePhysicalDevices failed, ErrorCode({})"_sv, result);

			core::Array<VkPhysicalDevice> physicalDevices{allocator};
			physicalDevices.resize_fill(physicalDeviceCount, VK_NULL_HANDLE);
			result = vkEnumeratePhysicalDevices(instance2, &physicalDeviceCount, physicalDevices.data());
			if (result != VK_SUCCESS)
				return core::errf(allocator, "vkEnumeratePhysicalDevices failed, ErrorCode({})"_sv, result);

			int maxScore = 0;
			for (auto device: physicalDevices)
			{
				auto requiredQueueFamiliesResult = getRequiredQueueFamilies(device, dummySurface2, allocator);
				if (requiredQueueFamiliesResult.isError())
					continue;
				requiredQueueFamilies2 = requiredQueueFamiliesResult.releaseValue();

				auto extensions = enumeratePhysicalDeviceExtensions(device, allocator);
				bool allExtensionsFound = true;
				for (auto ext: requiredDeviceExtensions)
				{
					if (findExtensionByName(extensions, core::StringView{ext}) == SIZE_MAX)
					{
						allExtensionsFound = false;
						break;
					}
				}

				if (allExtensionsFound == false)
					continue;

				auto swapchainSupport = SwapchainSupport2::query(device, dummySurface2, allocator);
				if (swapchainSupport.formats.count() == 0 || swapchainSupport.presentModes.count() == 0)
					continue;

				auto score = ratePhysicalDevice(device);
				if (score > maxScore)
				{
					maxScore = score;
					physicalDevice2 = device;
					swapchainSurfaceFormat = chooseSwapchainFormat(swapchainSupport.formats);
					swapchainPresentMode = choosePresentMode(swapchainSupport.presentModes);
				}
			}

			if (physicalDevice2 == VK_NULL_HANDLE)
				return core::errf(allocator, "failed to find suitable physical device"_sv);
		}

		// create logical device
		VkDevice logicalDevice2 = VK_NULL_HANDLE;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
		coreDefer
		{
			if (logicalDevice2 != VK_NULL_HANDLE)
				vkDestroyDevice(logicalDevice2, nullptr);
		};
		{
			float queuePriority = 1.0f;

			VkDeviceQueueCreateInfo graphicsQueueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = requiredQueueFamilies2.graphicsFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority,
			};

			VkDeviceQueueCreateInfo presentQueueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = requiredQueueFamilies2.presentFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority,
			};

			VkDeviceQueueCreateInfo queues[2] = {graphicsQueueCreateInfo, presentQueueCreateInfo};
			uint32_t queuesCount = 2;
			if (requiredQueueFamilies2.graphicsFamily == requiredQueueFamilies2.presentFamily)
				queuesCount = 1;

			VkPhysicalDeviceFeatures enabledFeatures{};

			VkDeviceCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.queueCreateInfoCount = queuesCount,
				.pQueueCreateInfos = queues,
				.enabledExtensionCount = sizeof(requiredDeviceExtensions) / sizeof(*requiredDeviceExtensions),
				.ppEnabledExtensionNames = requiredDeviceExtensions,
				.pEnabledFeatures = &enabledFeatures,
			};

			if (enableValidation)
			{
				createInfo.enabledLayerCount = sizeof(validationLayers2) / sizeof(*validationLayers2);
				createInfo.ppEnabledLayerNames = validationLayers2;
			}

			auto result = vkCreateDevice(physicalDevice2, &createInfo, nullptr, &logicalDevice2);
			if (result != VK_SUCCESS)
				return core::errf(allocator, "vkCreateDevice failed, ErrorCode({})"_sv, result);

			vkGetDeviceQueue(logicalDevice2, requiredQueueFamilies2.graphicsFamily, 0, &graphicsQueue);
			vkGetDeviceQueue(logicalDevice2, requiredQueueFamilies2.presentFamily, 0, &presentQueue);
		}

		auto renderer = core::unique_from<VkRenderer>(allocator, log, allocator);

		renderer->m_instance = instance2;
		instance2 = VK_NULL_HANDLE;

		renderer->m_debugMessenger = debugMessenger2;
		debugMessenger2 = VK_NULL_HANDLE;

		renderer->m_physicalDevice = physicalDevice2;
		physicalDevice2 = VK_NULL_HANDLE;

		renderer->m_swapchainSurfaceFormat = swapchainSurfaceFormat;
		renderer->m_swapchainPresentMode = swapchainPresentMode;

		renderer->m_logicalDevice = logicalDevice2;
		logicalDevice2 = VK_NULL_HANDLE;

		renderer->m_graphicsQueue = graphicsQueue;
		renderer->m_presentQueue = presentQueue;

		renderer->m_dummySurface = dummySurface2;
		dummySurface2 = VK_NULL_HANDLE;

		return renderer;
	}

	VkRenderer::~VkRenderer()
	{
		if (m_dummySurface != VK_NULL_HANDLE) vkDestroySurfaceKHR(m_instance, m_dummySurface, nullptr);
		if (m_logicalDevice != VK_NULL_HANDLE) vkDestroyDevice(m_logicalDevice, nullptr);
		if (m_debugMessenger != VK_NULL_HANDLE) vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		if (m_instance != VK_NULL_HANDLE) vkDestroyInstance(m_instance, nullptr);
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
#elif TAHA_OS_LINUX
		VkWaylandSurfaceCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
			.display = desc.display,
			.surface = desc.surface,
		};

		VkSurfaceKHR surface;
		auto result = vkCreateWaylandSurfaceKHR(m_instance, &createInfo, nullptr, &surface);
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
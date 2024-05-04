#include "taha/vk/VkRenderer.h"

#include <core/Array.h>

#if TAHA_OS_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR 1
#endif
#include <vulkan/vulkan.h>

namespace fmt
{
	template<>
	struct formatter<VkResult>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(VkResult r, FormatContext& ctx)
		{
			switch (r)
			{
			case VK_SUCCESS:
				return format_to(ctx.out(), "VK_SUCCESS");
			case VK_NOT_READY:
				return format_to(ctx.out(), "VK_NOT_READY");
			case VK_TIMEOUT:
				return format_to(ctx.out(), "VK_TIMEOUT");
			case VK_EVENT_SET:
				return format_to(ctx.out(), "VK_EVENT_SET");
			case VK_EVENT_RESET:
				return format_to(ctx.out(), "VK_EVENT_RESET");
			case VK_INCOMPLETE:
				return format_to(ctx.out(), "VK_INCOMPLETE");
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				return format_to(ctx.out(), "VK_ERROR_OUT_OF_HOST_MEMORY");
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				return format_to(ctx.out(), "VK_ERROR_OUT_OF_DEVICE_MEMORY");
			case VK_ERROR_INITIALIZATION_FAILED:
				return format_to(ctx.out(), "VK_ERROR_INITIALIZATION_FAILED");
			case VK_ERROR_DEVICE_LOST:
				return format_to(ctx.out(), "VK_ERROR_DEVICE_LOST");
			case VK_ERROR_MEMORY_MAP_FAILED:
				return format_to(ctx.out(), "VK_ERROR_MEMORY_MAP_FAILED");
			case VK_ERROR_LAYER_NOT_PRESENT:
				return format_to(ctx.out(), "VK_ERROR_LAYER_NOT_PRESENT");
			case VK_ERROR_EXTENSION_NOT_PRESENT:
				return format_to(ctx.out(), "VK_ERROR_EXTENSION_NOT_PRESENT");
			case VK_ERROR_FEATURE_NOT_PRESENT:
				return format_to(ctx.out(), "VK_ERROR_FEATURE_NOT_PRESENT");
			case VK_ERROR_INCOMPATIBLE_DRIVER:
				return format_to(ctx.out(), "VK_ERROR_INCOMPATIBLE_DRIVER");
			case VK_ERROR_TOO_MANY_OBJECTS:
				return format_to(ctx.out(), "VK_ERROR_TOO_MANY_OBJECTS");
			case VK_ERROR_FORMAT_NOT_SUPPORTED:
				return format_to(ctx.out(), "VK_ERROR_FORMAT_NOT_SUPPORTED");
			case VK_ERROR_FRAGMENTED_POOL:
				return format_to(ctx.out(), "VK_ERROR_FRAGMENTED_POOL");
			case VK_ERROR_UNKNOWN:
			default:
				return format_to(ctx.out(), "VK_ERROR_UNKNOWN");
			}
		}
	};
}

namespace taha
{
	core::Result<core::Unique<VkRenderer>> VkRenderer::create(core::Allocator *allocator)
	{
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "No App",
			.applicationVersion = VK_MAKE_VERSION(0, 0, 0),
			.pEngineName = "Taha Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_0,
		};

		// layers
		const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

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
		const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
		#endif

		uint32_t availableExtensionsCount = 0;
		result = vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, nullptr);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkEnumerateInstanceExtensionProperties failed, ErrorCode({})"_sv, result);

		core::Array<VkExtensionProperties> availableExtensions{allocator};
		availableExtensions.resize_fill(availableExtensionsCount, VkExtensionProperties{});
		result = vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionsCount, availableExtensions.begin());
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

		// create instance
		VkInstanceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = sizeof(validationLayers) / sizeof(*validationLayers),
			.ppEnabledLayerNames = validationLayers,
			.enabledExtensionCount = sizeof(extensions) / sizeof(*extensions),
			.ppEnabledExtensionNames = extensions,
		};

		VkInstance instance{};
		result = vkCreateInstance(&createInfo, nullptr, &instance);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkCreateInstance failed, ErrorCode({})"_sv, result);

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0)
			return core::errf(allocator, "Failed to find GPUs with Vulkan support"_sv);
		core::Array<VkPhysicalDevice> devices{allocator};
		devices.resize_fill(deviceCount, {});
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.begin());

		// TODO: check all devices and choose the best one
		VkPhysicalDevice physicalDevice = devices[0];
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = 0,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority,
		};

		VkDeviceCreateInfo deviceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &queueCreateInfo,
		};

		VkDevice device{};
		result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
		if (result != VK_SUCCESS)
			return core::errf(allocator, "vkCreateDevice failed, ErrorCode({})"_sv, result);

		return nullptr;
	}

	core::Unique<Frame> VkRenderer::createFrameForWindow(NativeWindowDesc desc)
	{
		return nullptr;
	}

	void VkRenderer::submitCommandsAndExecute(Frame *frame, const core::Array<core::Unique<Command>> &commands)
	{
		// do nothing
	}
}
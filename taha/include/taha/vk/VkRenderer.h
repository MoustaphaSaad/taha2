#pragma once

#include "taha/Exports.h"
#include "taha/Renderer.h"

#include <core/Result.h>
#include <core/Unique.h>
#include <core/Log.h>

#if TAHA_OS_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR 1
#endif
#include <vulkan/vulkan.h>

namespace taha
{
	class VkFrame;

	class VkRenderer: public Renderer
	{
		friend class VkFrame;

		template<typename T, typename... TArgs>
		friend inline core::Unique<T> core::unique_from(core::Allocator* allocator, TArgs&&... args);

		core::Allocator* m_allocator = nullptr;
		core::Log* m_log = nullptr;
		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		PFN_vkDestroyDebugUtilsMessengerEXT m_destroyDebugMessengerFn = nullptr;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkQueue m_graphicsQueue = VK_NULL_HANDLE;

		VkRenderer(core::Log* log, core::Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log)
		{}

		TAHA_EXPORT static VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
			void* userData);
	public:
		TAHA_EXPORT static core::Result<core::Unique<VkRenderer>> create(core::Log* log, core::Allocator* allocator);

		TAHA_EXPORT ~VkRenderer() override;

		TAHA_EXPORT core::Unique<Frame> createFrameForWindow(NativeWindowDesc desc) override;
		TAHA_EXPORT void submitCommandsAndExecute(Frame* frame, const core::Array<core::Unique<Command>>& commands) override;
	};
}
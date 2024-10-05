#pragma once

#include <volk.h>

#include "taha/Exports.h"
#include "taha/Renderer.h"

#include <core/Log.h>
#include <core/Result.h>
#include <core/Unique.h>

namespace taha
{
	class VkFrame;

	class VkRenderer: public Renderer
	{
		friend class VkFrame;

		template <typename T, typename... TArgs>
		friend inline core::Unique<T> core::unique_from(core::Allocator* allocator, TArgs&&... args);

		core::Allocator* m_allocator = nullptr;
		core::Log* m_log = nullptr;
		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		uint32_t m_graphicsQueueFamily = 0;
		uint32_t m_presentQueueFamily = 0;
		VkDevice m_logicalDevice = VK_NULL_HANDLE;
		VkQueue m_graphicsQueue = VK_NULL_HANDLE;
		VkQueue m_presentQueue = VK_NULL_HANDLE;
		VkSurfaceKHR m_dummySurface = VK_NULL_HANDLE;

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
		TAHA_EXPORT void
		submitCommandsAndExecute(Frame* frame, const core::Array<core::Unique<Command>>& commands) override;
	};
}
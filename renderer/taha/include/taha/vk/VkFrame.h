#pragma once

#include "taha/Exports.h"
#include "taha/Frame.h"
#include "taha/vk/VkRenderer.h"

namespace taha
{
	class VkFrame: public Frame
	{
		friend class VkRenderer;

		VkRenderer* m_renderer = nullptr;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
		core::Array<VkImage> m_swapchainImages;
		core::Array<VkImageView> m_swapchainImageViews;

		TAHA_EXPORT void endEncodingAndSubmit(const core::Array<core::Unique<Command>>& commands) override;

	public:
		VkFrame(
			VkRenderer* renderer,
			VkSurfaceKHR surface,
			VkSwapchainKHR swapchain,
			core::Array<VkImage> swapchainImages,
			core::Array<VkImageView> swapchainImageViews)
			: m_renderer(renderer),
			  m_surface(surface),
			  m_swapchain(swapchain),
			  m_swapchainImages(std::move(swapchainImages)),
			  m_swapchainImageViews(std::move(swapchainImageViews))
		{}

		TAHA_EXPORT ~VkFrame() override;
		TAHA_EXPORT Encoder createEncoderAndRecord(const FrameAction& action, core::Allocator* allocator) override;
	};
}
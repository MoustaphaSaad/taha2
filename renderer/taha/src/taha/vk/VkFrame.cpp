#include "taha/vk/VkFrame.h"

namespace taha
{
	void VkFrame::endEncodingAndSubmit(const core::Array<core::Unique<Command>>& commands)
	{
		m_renderer->submitCommandsAndExecute(this, commands);
	}

	VkFrame::~VkFrame()
	{
		for (auto view: m_swapchainImageViews)
		{
			vkDestroyImageView(m_renderer->m_logicalDevice, view, nullptr);
		}

		if (m_swapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(m_renderer->m_logicalDevice, m_swapchain, nullptr);
		}

		if (m_surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_renderer->m_instance, m_surface, nullptr);
		}
	}

	Encoder VkFrame::createEncoderAndRecord(const FrameAction& action, core::Allocator* allocator)
	{
		return Encoder{this, action, allocator};
	}
}
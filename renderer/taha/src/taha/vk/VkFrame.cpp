#include "taha/vk/VkFrame.h"

namespace taha
{
	void VkFrame::endEncodingAndSubmit(const core::Array<core::Unique<Command>>& commands)
	{
		m_renderer->submitCommandsAndExecute(this, commands);
	}

	VkFrame::~VkFrame()
	{
		if (m_surface != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(m_renderer->m_instance, m_surface, nullptr);
	}

	Encoder VkFrame::createEncoderAndRecord(const FrameAction& action, core::Allocator* allocator)
	{
		return Encoder{this, action, allocator};
	}
}
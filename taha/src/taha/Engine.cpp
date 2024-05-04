#include "taha/Engine.h"

#if TAHA_OS_WINDOWS
#include "taha/dx11/DX11Renderer.h"
#include "taha/vk/VkRenderer.h"
#endif

namespace taha
{
	core::Result<Engine> Engine::create(API api, core::Log* log, core::Allocator* allocator)
	{
		core::Unique<Renderer> renderer{};
		#if TAHA_OS_WINDOWS
			switch (api)
			{
			case API_DX11:
			{
				auto rendererResult = DX11Renderer::create(allocator);
				if (rendererResult.isError())
					return rendererResult.releaseError();
				renderer = rendererResult.releaseValue();
				break;
			}
			case API_VULKAN:
			{
				auto rendererResult = VkRenderer::create(log, allocator);
				if (rendererResult.isError())
					return rendererResult.releaseError();
				renderer = rendererResult.releaseValue();
				break;
			}
			}
		#endif

		return Engine{std::move(renderer)};
	}

	core::Unique<Frame> Engine::createFrameForWindow(taha::NativeWindowDesc desc)
	{
		return m_renderer->createFrameForWindow(desc);
	}
}
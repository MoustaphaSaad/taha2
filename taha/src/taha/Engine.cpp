#include "taha/Engine.h"

#if TAHA_OS_WINDOWS
#include "taha/dx11/DX11Renderer.h"
#endif

namespace taha
{
	core::Result<Engine> Engine::create(core::Allocator* allocator)
	{
		#if TAHA_OS_WINDOWS
			auto rendererResult = DX11Renderer::create(allocator);
		#endif

		if (rendererResult.isError())
			return rendererResult.releaseError();
		return Engine{rendererResult.releaseValue()};
	}

	core::Unique<Frame> Engine::createFrameForWindow(taha::NativeWindowDesc desc)
	{
		return m_renderer->createFrameForWindow(desc);
	}
}
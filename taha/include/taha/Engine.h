#pragma once

#include "taha/Exports.h"
#include "taha/Frame.h"
#include "taha/Renderer.h"
#include "taha/NativeWindowDesc.h"

#include <core/Result.h>
#include <core/Unique.h>

namespace taha
{
	class Engine
	{
		core::Unique<Renderer> m_renderer;

		explicit Engine(core::Unique<Renderer> renderer)
			: m_renderer(std::move(renderer))
		{}

	public:
		TAHA_EXPORT static core::Result<Engine> create(core::Allocator* allocator);

		TAHA_EXPORT core::Unique<Frame> createFrameForWindow(NativeWindowDesc desc);

	};
}
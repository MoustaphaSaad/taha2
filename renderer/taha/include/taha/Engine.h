#pragma once

#include "taha/Exports.h"
#include "taha/Frame.h"
#include "taha/NativeWindowDesc.h"
#include "taha/Renderer.h"

#include <core/Log.h>
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
		enum API
		{
			API_DX11,
			API_VULKAN,
		};

		TAHA_EXPORT static core::Result<Engine> create(API api, core::Log* log, core::Allocator* allocator);

		TAHA_EXPORT core::Unique<Frame> createFrameForWindow(NativeWindowDesc desc);
	};
}
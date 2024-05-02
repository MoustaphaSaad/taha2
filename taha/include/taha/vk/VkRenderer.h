#pragma once

#include "taha/Exports.h"
#include "taha/Renderer.h"

#include <core/Result.h>
#include <core/Unique.h>

namespace taha
{
	class VkRenderer: public Renderer
	{
		template<typename T, typename... TArgs>
		friend inline core::Unique<T> core::unique_from(core::Allocator* allocator, TArgs&&... args);

		core::Allocator* m_allocator = nullptr;

		VkRenderer(core::Allocator* allocator)
			: m_allocator(allocator)
		{}

	public:
		TAHA_EXPORT static core::Result<core::Unique<VkRenderer>> create(core::Allocator* allocator);

		TAHA_EXPORT core::Unique<Frame> createFrameForWindow(NativeWindowDesc desc) override;
		TAHA_EXPORT void submitCommandsAndExecute(Frame* frame, const core::Array<core::Unique<Command>>& commands) override;
	};
}
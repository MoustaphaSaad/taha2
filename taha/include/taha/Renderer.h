#pragma once

#include "taha/Frame.h"
#include "taha/NativeWindowDesc.h"

#include <core/Unique.h>

namespace taha
{
	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual core::Unique<Frame> createFrameForWindow(NativeWindowDesc desc) = 0;
	};
}
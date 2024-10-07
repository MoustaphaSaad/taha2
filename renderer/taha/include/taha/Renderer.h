#pragma once

#include "taha/Encoder.h"
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
		virtual void submitCommandsAndExecute(Frame* frame, const core::Array<core::Unique<Command>>& commands) = 0;
	};
}
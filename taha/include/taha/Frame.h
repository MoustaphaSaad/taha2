#pragma once

#include "taha/Encoder.h"

namespace taha
{
	class Frame
	{
		friend class Encoder;

		virtual void endEncodingAndSubmit(const core::Array<core::Unique<Command>>& commands) = 0;
	public:
		virtual ~Frame() = default;
		virtual Encoder createEncoderAndRecord(math::rgba clearColor, core::Allocator* allocator) = 0;
	};
}
#pragma once

#include "taha/Exports.h"
#include "taha/Command.h"

#include <core/Array.h>
#include <core/Unique.h>

namespace taha
{
	class Frame;

	class Encoder
	{
		core::Allocator* m_allocator = nullptr;
		core::Array<core::Unique<Command>> m_commands;
		Frame* m_frame = nullptr;
	public:
		TAHA_EXPORT Encoder(Frame* frame, const FrameAction& action, core::Allocator* allocator);
		Encoder(const Encoder&) = delete;
		Encoder& operator=(const Encoder&) = delete;

		TAHA_EXPORT void endEncodingAndSubmit();
	};
}
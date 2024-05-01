#include "taha/Encoder.h"
#include "taha/Frame.h"

namespace taha
{
	Encoder::Encoder(Frame* frame, const FrameAction& action, core::Allocator* allocator)
		: m_allocator(allocator),
		  m_commands(allocator),
		  m_frame(frame)
	{
		auto clear = core::unique_from<ClearCommand>(allocator);
		clear->action = action;
		m_commands.push(std::move(clear));
	}

	void Encoder::endEncodingAndSubmit()
	{
		m_frame->endEncodingAndSubmit(m_commands);
	}
}
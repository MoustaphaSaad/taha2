#include "taha/Engine.h"

namespace taha
{
	core::Result<Engine> Engine::create(core::Allocator* allocator)
	{
		return core::errf(allocator, "not implemented"_sv);
	}

	Frame Engine::createFrame()
	{
		return Frame{};
	}
}
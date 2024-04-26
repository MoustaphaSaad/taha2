#pragma once

#include "taha/Exports.h"
#include "taha/Frame.h"

#include <core/Result.h>

namespace taha
{
	class Engine
	{
	public:
		static core::Result<Engine> create(core::Allocator* allocator);

		Frame createFrame();
	};
}
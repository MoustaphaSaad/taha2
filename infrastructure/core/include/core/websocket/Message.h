#pragma once

#include "core/Buffer.h"

namespace core::websocket
{
	struct Message
	{
		enum TYPE
		{
			TYPE_NONE,
			TYPE_TEXT,
			TYPE_BINARY,
			TYPE_CLOSE,
			TYPE_PING,
			TYPE_PONG,
		};

		TYPE type = TYPE_NONE;
		Buffer payload;

		explicit Message(Allocator* allocator)
			: payload(allocator)
		{}
	};
}
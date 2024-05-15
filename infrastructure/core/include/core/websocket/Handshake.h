#pragma once

#include "core/Exports.h"
#include "core/String.h"
#include "core/Result.h"

namespace core::websocket
{
	class Handshake
	{
		String m_key;
	public:
		explicit Handshake(Allocator* allocator)
			: m_key(allocator)
		{}

		explicit Handshake(String key)
			: m_key(key)
		{}

		StringView key() const { return m_key; }

		CORE_EXPORT static Result<Handshake> parse(StringView request, Allocator* allocator);
		CORE_EXPORT static Result<Handshake> parseResponse(StringView response, Allocator* allocator);
	};
}
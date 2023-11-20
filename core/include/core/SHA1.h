#pragma once

#include "core/Exports.h"
#include "core/Span.h"
#include "core/String.h"

#include <fmt/core.h>

namespace core
{
	class SHA1
	{
		std::byte m_digest[20];
	public:
		CORE_EXPORT static SHA1 hash(Span<std::byte> bytes);

		template<typename T>
		static SHA1 hash(Span<T> s)
		{
			return hash(s.asBytes());
		}

		static SHA1 hash(StringView str)
		{
			return hash(Span<std::byte>{str});
		}

		Span<std::byte> asBytes()
		{
			return {m_digest, 20};
		}

		Span<const std::byte> asBytes() const
		{
			return {m_digest, 20};
		}
	};
}

namespace fmt
{
	template<>
	struct formatter<core::SHA1>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const core::SHA1& str, FormatContext& ctx)
		{
			auto bytes = str.asBytes();
			for (size_t i = 0; i < 20; ++i)
			{
				fmt::format_to(ctx.out(), "{:02x}", (int)bytes[i]);
			}
			return ctx.out();
		}
	};
}
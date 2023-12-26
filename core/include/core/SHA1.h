#pragma once

#include "core/Exports.h"
#include "core/Span.h"
#include "core/String.h"

#include <fmt/core.h>

#include <openssl/sha.h>

namespace core
{
	class SHA1Hasher;

	class SHA1
	{
		friend class SHA1Hasher;
		std::byte m_digest[20];
	public:
		static SHA1 hash(Span<std::byte> bytes)
		{
			SHA_CTX ctx;
			SHA1_Init(&ctx);
			SHA1_Update(&ctx, bytes.data(), bytes.count());

			SHA1 sha{};
			SHA1_Final((unsigned char*)sha.m_digest, &ctx);

			return sha;
		}

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

	class SHA1Hasher
	{
		SHA_CTX m_ctx;
	public:
		SHA1Hasher()
		{
			SHA1_Init(&m_ctx);
		}

		void hash(Span<std::byte> bytes)
		{
			SHA1_Update(&m_ctx, bytes.data(), bytes.count());
		}

		template<typename T>
		void hash(Span<T> s)
		{
			hash(s.asBytes());
		}

		void hash(StringView str)
		{
			hash(Span<std::byte>{str});
		}

		SHA1 final()
		{
			SHA1 res{};
			SHA1_Final((unsigned char*)res.m_digest, &m_ctx);
			return res;
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
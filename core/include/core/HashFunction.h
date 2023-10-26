#pragma once

#include "core/String.h"
#include "core/Buffer.h"
#include "core/UUID.h"
#include "core/Unique.h"

namespace core
{
	template<typename T>
	struct Hash
	{
		inline size_t operator()(const T&) const
		{
			static_assert(sizeof(T) == 0, "Hash not implemented for this type");
			return 0;
		}
	};

	template<typename T>
	struct Hash<T*>
	{
		inline size_t operator()(const T* ptr) const
		{
			return (size_t)ptr;
		}
	};

	template<>
	struct Hash<bool>
	{
		inline size_t operator()(bool value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<char>
	{
		inline size_t operator()(char value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<short>
	{
		inline size_t operator()(short value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<int>
	{
		inline size_t operator()(int value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<long>
	{
		inline size_t operator()(long value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<long long>
	{
		inline size_t operator()(long long value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned char>
	{
		inline size_t operator()(unsigned char value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned short>
	{
		inline size_t operator()(unsigned short value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned int>
	{
		inline size_t operator()(unsigned int value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned long>
	{
		inline size_t operator()(unsigned long value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned long long>
	{
		inline size_t operator()(unsigned long long value) const
		{
			return size_t(value);
		}
	};

	template<typename T>
	struct Hash<Unique<T>>
	{
		inline size_t operator()(const Unique<T>& value) const
		{
			return Hash<T*>{}(value.get());
		}
	};

	inline static size_t _MurmurHashUnaligned2(const void* ptr, size_t len, size_t seed)
	{
		if constexpr (sizeof(void*) == 4)
		{
			const size_t m = 0x5bd1e995;
			size_t hash = seed ^ len;
			const unsigned char* buffer = static_cast<const unsigned char*>(ptr);

			while (len >= 4)
			{
				size_t k = *reinterpret_cast<const size_t*>(buffer);
				k *= m;
				k ^= k >> 24;
				k *= m;
				hash *= m;
				hash ^= k;
				buffer += 4;
				len -= 4;
			}

			if (len == 3)
			{
				hash ^= static_cast<unsigned char>(buffer[2]) << 16;
				--len;
			}

			if (len == 2)
			{
				hash ^= static_cast<unsigned char>(buffer[1]) << 8;
				--len;
			}

			if (len == 1)
			{
				hash ^= static_cast<unsigned char>(buffer[0]);
				hash *= m;
				--len;
			}

			hash ^= hash >> 13;
			hash *= m;
			hash ^= hash >> 15;
			return hash;
		}
		else if constexpr (sizeof(void*) == 8)
		{
			auto load_bytes = [](const unsigned char* p, intptr_t n) -> size_t
			{
				size_t result = 0;
				--n;
				do
					result = (result << 8) + static_cast<unsigned char>(p[n]);
				while (--n >= 0);
				return result;
			};

			auto shift_mix = [](size_t v) -> size_t
			{
				return v ^ (v >> 47);
			};

			static const size_t mul = (static_cast<size_t>(0xc6a4a793UL) << 32UL) + static_cast<size_t>(0x5bd1e995UL);

			const unsigned char* const buffer = static_cast<const unsigned char*>(ptr);

			const int32_t len_aligned = len & ~0x7;
			const unsigned char* const end = buffer + len_aligned;

			size_t hash = seed ^ (len * mul);
			for (const unsigned char* p = buffer;
				p != end;
				p += 8)
			{
				const size_t data = shift_mix((*reinterpret_cast<const size_t*>(p)) * mul) * mul;
				hash ^= data;
				hash *= mul;
			}

			if ((len & 0x7) != 0)
			{
				const size_t data = load_bytes(end, len & 0x7);
				hash ^= data;
				hash *= mul;
			}

			hash = shift_mix(hash) * mul;
			hash = shift_mix(hash);
			return hash;
		}
	}

	// hashes a block of bytes using murmur hash algorithm
	inline static size_t murmurHash(const void* ptr, size_t len, size_t seed = 0xc70f6907UL)
	{
		return _MurmurHashUnaligned2(ptr, len, seed);
	}

	template<>
	struct Hash<float>
	{
		inline size_t operator()(float value) const
		{
			return value != 0.0f ? murmurHash(&value, sizeof(float)) : 0;
		}
	};

	template<>
	struct Hash<double>
	{
		inline size_t operator()(double value) const
		{
			return value != 0.0f ? murmurHash(&value, sizeof(double)) : 0;
		}
	};

	template<>
	struct Hash<String>
	{
		inline size_t operator()(const String& value) const
		{
			return murmurHash(value.data(), value.count());
		}
	};

	template<>
	struct Hash<StringView>
	{
		inline size_t operator()(StringView value) const
		{
			return murmurHash(value.data(), value.count());
		}
	};

	template<>
	struct Hash<Buffer>
	{
		inline size_t operator()(const Buffer& value) const
		{
			return murmurHash(value.data(), value.count());
		}
	};

	template<>
	struct Hash<UUID>
	{
		inline size_t operator()(UUID value) const
		{
			return murmurHash(&value, sizeof(value));
		}
	};

	// mixes two hash values together
	inline static size_t hashMix(size_t a, size_t b)
	{
		if constexpr (sizeof(size_t) == 4)
		{
			return (b + 0x9e3779b9 + (a << 6) + (a >> 2));
		}
		else if constexpr (sizeof(size_t) == 8)
		{
			a ^= b;
			a *= 0xff51afd7ed558ccd;
			a ^= a >> 32;
			return a;
		}
	}
}
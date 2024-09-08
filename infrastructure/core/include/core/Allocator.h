#pragma once

#include "core/Span.h"

#include <cstddef>

namespace core
{
	class Allocator
	{
	public:
		virtual ~Allocator() = default;

		virtual Span<std::byte> alloc(size_t size, size_t alignment) = 0;
		virtual void commit(Span<std::byte> bytes) = 0;
		virtual void release(Span<std::byte> bytes) = 0;
		virtual void free(Span<std::byte> bytes) = 0;

		template<typename T>
		Span<T> allocT(size_t count)
		{
			auto bytes = alloc(count * sizeof(T), alignof(T));
			return Span<T>{(T*)bytes.data(), bytes.sizeInBytes() / sizeof(T)};
		}

		template<typename T>
		void commitT(Span<T> s)
		{
			commit(s.asBytes());
		}

		template<typename T>
		void releaseT(Span<T> s)
		{
			release(s.asBytes());
		}

		template<typename T>
		void freeT(Span<T> s)
		{
			free(s.asBytes());
		}

		template<typename T>
		T* allocSingleT()
		{
			return allocT<T>(1).data();
		}

		template<typename T>
		void commitSingleT(T* s)
		{
			commitT(Span<T>{s, 1});
		}

		template<typename T>
		void releaseSingleT(T* s)
		{
			releaseT(Span<T>{s, 1});
		}

		template<typename T>
		void freeSingleT(T* s)
		{
			freeT(Span<T>{s, 1});
		}
	};
}

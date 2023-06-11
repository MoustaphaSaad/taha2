#pragma once

#include "core/Allocator.h"

#include <utility>
#include <new>

namespace core
{
	template<typename T>
	class Unique
	{
		Allocator* allocator = nullptr;
		T* ptr = nullptr;

		void destroy()
		{
			if (ptr)
			{
				ptr->~T();
				allocator->release(ptr, sizeof(T));
				allocator->free(ptr, sizeof(T));
				ptr = nullptr;
			}
		}

		void moveFrom(Unique&& other)
		{
			allocator = other.allocator;
			ptr = other.ptr;
			other.ptr = nullptr;
		}

	public:
		Unique() = default;

		Unique(Allocator* a, T* p)
			: allocator(a),
			  ptr(p)
		{}

		Unique(std::nullptr_t){}

		Unique(const Unique&) = delete;

		Unique(Unique&& other)
			: allocator(other.allocator),
			  ptr(other.ptr)
		{
			other.ptr = nullptr;
		}

		Unique& operator=(std::nullptr_t)
		{
			destroy();
			ptr = nullptr;
			return *this;
		}

		Unique& operator=(const Unique&) = delete;

		Unique& operator=(Unique&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~Unique()
		{
			destroy();
		}

		const T& operator*() const { return *ptr; }
		T& operator*() { return *ptr; }

		const T* operator->() const { return ptr; }
		T* operator->() { return ptr; }

		operator bool() const { return ptr != nullptr; }

		T* get() const { return ptr; }

		T* leak()
		{
			auto p = ptr;
			ptr = nullptr;
			return p;
		}
	};

	template<typename T, typename... TArgs>
	inline Unique<T>
	unique_from(Allocator* allocator, TArgs&&... args)
	{
		auto ptr = (T*)allocator->alloc(sizeof(T), alignof(T));
		allocator->commit(ptr, sizeof(T));
		::new (ptr) T(std::forward<TArgs>(args)...);
		return Unique<T>{allocator, (T*)ptr};
	}
}
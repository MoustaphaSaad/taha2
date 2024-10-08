#pragma once

#include "core/Allocator.h"

#include <new>
#include <utility>

namespace core
{
	template <typename T>
	class Unique
	{
		Allocator* m_allocator = nullptr;
		T* m_ptr = nullptr;

		void destroy()
		{
			if (m_ptr)
			{
				m_ptr->~T();
				m_allocator->releaseSingleT(m_ptr);
				m_allocator->freeSingleT(m_ptr);
				m_ptr = nullptr;
			}
		}

		template <typename U>
		void moveFrom(Unique<U>& other)
		{
			m_allocator = other.allocator();
			m_ptr = other.leak();
		}

	public:
		Unique() = default;

		Unique(Allocator* a, T* p)
			: m_allocator(a),
			  m_ptr(p)
		{}

		Unique(std::nullptr_t) {}

		Unique(const Unique&) = delete;

		Unique(Unique&& other) noexcept
			: m_allocator(other.allocator()),
			  m_ptr(other.leak())
		{}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		Unique(Unique<U>&& other) noexcept
			: m_allocator(other.allocator()),
			  m_ptr(other.leak())
		{}

		Unique& operator=(std::nullptr_t)
		{
			destroy();
			m_ptr = nullptr;
			return *this;
		}

		Unique& operator=(const Unique&) = delete;

		Unique& operator=(Unique&& other) noexcept
		{
			destroy();
			moveFrom(other);
			return *this;
		}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		Unique& operator=(Unique<U>&& other) noexcept
		{
			destroy();
			moveFrom(other);
			return *this;
		}

		~Unique()
		{
			destroy();
		}

		T& operator*() const
		{
			return *m_ptr;
		}
		T* operator->() const
		{
			return m_ptr;
		}

		operator bool() const
		{
			return m_ptr != nullptr;
		}
		bool operator==(std::nullptr_t) const
		{
			return m_ptr == nullptr;
		}
		bool operator!=(std::nullptr_t) const
		{
			return m_ptr != nullptr;
		}
		template <typename R>
		bool operator==(const Unique<R>& other) const
		{
			return m_ptr == other.m_ptr;
		}
		template <typename R>
		bool operator!=(const Unique<R>& other) const
		{
			return m_ptr != other.m_ptr;
		}

		T* get() const
		{
			return m_ptr;
		}

		[[nodiscard]] T* leak()
		{
			auto p = m_ptr;
			m_ptr = nullptr;
			return p;
		}

		Allocator* allocator() const
		{
			return m_allocator;
		}
	};

	template <typename T, typename... TArgs>
	inline Unique<T> unique_from(Allocator* allocator, TArgs&&... args)
	{
		auto ptr = allocator->allocSingleT<T>();
		allocator->commitSingleT(ptr);
		::new (ptr) T(std::forward<TArgs>(args)...);
		return Unique<T>{allocator, (T*)ptr};
	}

	template <typename T, typename R>
	inline Unique<T> unique_static_cast(Unique<R> other)
	{
		return Unique<T>{other.allocator(), static_cast<T*>(other.leak())};
	}
}
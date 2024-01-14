#pragma once

#include "core/Allocator.h"
#include "core/HashFunction.h"

#include <atomic>
#include <cassert>
#include <new>

namespace core
{
	template<typename T>
	class Weak;

	template<typename T>
	class Shared
	{
		template<typename>
		friend class Shared;

		template<typename TPtr, typename... TArgs>
		friend inline Shared<TPtr>
		shared_from(Allocator* allocator, TArgs&&... args);

		friend class Weak<T>;

		struct Control
		{
			std::atomic<int> strong = 0;
			std::atomic<int> weak = 0;
			T* ptr = nullptr;
		};

		Allocator* m_allocator = nullptr;
		Control* m_control = nullptr;

		void ref()
		{
			if (m_control)
			{
				m_control->strong.fetch_add(1);
				m_control->weak.fetch_add(1);
			}
		}

		void unref()
		{
			if (m_control)
			{
				if (m_control->strong.fetch_sub(1) == 1)
				{
					m_control->ptr->~T();
					m_allocator->release(m_control->ptr, sizeof(T));
					m_allocator->free(m_control->ptr, sizeof(T));
					m_control->ptr = nullptr;

					if (m_control->weak.fetch_sub(1) == 1)
					{
						m_allocator->release(m_control, sizeof(Control));
						m_allocator->free(m_control, sizeof(Control));
						m_control = nullptr;
					}
				}
			}
		}

		template<typename U>
		void copyFrom(const Shared<U>& other)
		{
			m_allocator = other.m_allocator;
			m_control = (Shared<T>::Control*)other.m_control;
			ref();
		}

		template<typename U>
		void moveFrom(Shared<U>&& other)
		{
			m_allocator = other.m_allocator;
			m_control = (Shared<T>::Control*)other.m_control;
			other.m_allocator = nullptr;
			other.m_control = nullptr;
		}

	public:
		Shared() = default;

		Shared(Allocator* a, Control* c)
			: m_allocator(a),
			  m_control(c)
		{
			ref();
		}

		Shared(std::nullptr_t){}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Shared(const Shared<U>& other)
		{
			copyFrom(other);
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Shared(Shared<U>&& other)
		{
			moveFrom(std::move(other));
		}

		Shared& operator=(std::nullptr_t)
		{
			unref();
			m_allocator = nullptr;
			m_control = nullptr;
			return *this;
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Shared& operator=(Shared<U>&& other)
		{
			unref();
			moveFrom(std::move(other));
			return *this;
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Shared& operator=(const Shared<U>& other)
		{
			unref();
			copyFrom(std::move(other));
			return *this;
		}

		~Shared()
		{
			unref();
		}

		const T& operator*() const
		{
			return *m_control->ptr;
		}

		T& operator*()
		{
			return *m_control->ptr;
		}

		const T* operator->() const
		{
			if (m_control)
				return m_control->ptr;
			else
				return nullptr;
		}

		T* operator->()
		{
			if (m_control)
				return m_control->ptr;
			else
				return nullptr;
		}

		operator bool() const
		{
			return m_control != nullptr;
		}

		bool operator==(std::nullptr_t) const { return m_control == nullptr; }
		bool operator!=(std::nullptr_t) const { return m_control != nullptr; }
		template<typename R>
		bool operator==(const Shared<R>& other) const { return m_control == other.m_control; }
		template<typename R>
		bool operator!=(const Shared<R>& other) const { return m_control != other.m_control; }

		int ref_count() const
		{
			if (m_control == nullptr)
				return 0;
			return m_control->strong.load();
		}

		T* get() const
		{
			if (m_control)
				return m_control->ptr;
			else
				return nullptr;
		}

		T* leak()
		{
			T* res = nullptr;
			if (m_control)
				res = m_control->ptr;
			m_control = nullptr;
			m_allocator = nullptr;
			return res;
		}

		Allocator* allocator() const { return m_allocator; }
	};

	template<typename T>
	struct Hash<Shared<T>>
	{
		inline size_t operator()(const Shared<T>& value) const
		{
			return Hash<T*>{}(value.get());
		}
	};

	template<typename T>
	class Weak
	{
		friend struct Hash<Weak<T>>;

		Allocator* allocator = nullptr;
		typename Shared<T>::Control* control = nullptr;

		void ref()
		{
			if (control)
				control->weak.fetch_add(1);
		}

		void unref()
		{
			if (control)
			{
				if (control->weak.fetch_sub(1) == 1)
				{
					allocator->release(control, sizeof(*control));
					allocator->free(control, sizeof(*control));
					control = nullptr;
				}
			}
		}

		void copyFrom(const Weak& other)
		{
			allocator = other.allocator;
			control = other.control;
			ref();
		}

		void moveFrom(Weak&& other)
		{
			allocator = other.allocator;
			control = other.control;

			other.allocator = nullptr;
			other.control = nullptr;
		}

	public:
		Weak() = default;

		Weak(const Shared<T>& shared)
		{
			allocator = shared.m_allocator;
			control = shared.m_control;
			ref();
		}

		Weak(const Weak& other)
		{
			copyFrom(other);
		}

		Weak(Weak&& other)
		{
			moveFrom(std::move(other));
		}

		Weak& operator=(const Weak& other)
		{
			unref();
			copyFrom(other);
			return *this;
		}

		Weak& operator=(Weak&& other)
		{
			unref();
			moveFrom(std::move(other));
			return *this;
		}

		~Weak()
		{
			unref();
		}

		operator bool() const
		{
			return control != nullptr;
		}

		bool operator==(std::nullptr_t) const { return control == nullptr; }
		bool operator!=(std::nullptr_t) const { return control != nullptr; }
		template<typename R>
		bool operator==(const Weak<R>& other) const { return control == other.control; }
		template<typename R>
		bool operator!=(const Weak<R>& other) const { return control != other.control; }

		int ref_count() const
		{
			if (control == nullptr)
				return 0;
			return control->strong.load();
		}

		bool expired() const
		{
			return ref_count() <= 0;
		}

		Shared<T> lock() const
		{
			if (control)
			{
				auto count = control->strong.load();
				while (count != 0)
				{
					if (control->strong.compare_exchange_weak(count, count + 1))
					{
						Shared<T> res{};
						res.m_allocator = allocator;
						res.m_control = control;
						return res;
					}

					count = control->strong.load();
				}
			}

			return Shared<T>{};
		}
	};

	template<typename T>
	struct Hash<Weak<T>>
	{
		inline size_t operator()(const Weak<T>& value) const
		{
			return Hash<void*>{}(value.control);
		}
	};

	template<typename T, typename ... TArgs>
	inline Shared<T>
	shared_from(Allocator* allocator, TArgs&& ... args)
	{
		auto ptr = (T*)allocator->alloc(sizeof(T), alignof(T));
		allocator->commit(ptr, sizeof(*ptr));

		using Control = typename Shared<T>::Control;

		auto control = (Control*)allocator->alloc(sizeof(Control), alignof(Control));
		allocator->commit(control, sizeof(*control));

		::new (ptr) T{std::forward<TArgs>(args)...};
		::new (control) Control{};
		control->ptr = ptr;
		return Shared<T>{allocator, control};
	}
}
#pragma once

#include "core/Allocator.h"

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

		Allocator* allocator = nullptr;
		Control* control = nullptr;

		void ref()
		{
			if (control)
			{
				control->strong.fetch_add(1);
				control->weak.fetch_add(1);
			}
		}

		void unref()
		{
			if (control)
			{
				if (control->strong.fetch_sub(1) == 1)
				{
					control->ptr->~T();
					allocator->release(control->ptr, sizeof(T));
					allocator->free(control->ptr, sizeof(T));
					control->ptr = nullptr;

					if (control->weak.fetch_sub(1) == 1)
					{
						allocator->release(control, sizeof(Control));
						allocator->free(control, sizeof(Control));
						control = nullptr;
					}
				}
			}
		}

		void copyFrom(const Shared& other)
		{
			allocator = other.allocator;
			control = other.control;
			ref();
		}

		void moveFrom(Shared&& other)
		{
			allocator = other.allocator;
			control = other.control;
			other.allocator = nullptr;
			other.control = nullptr;
		}

	public:
		Shared() = default;

		Shared(Allocator* a, Control* c)
			: allocator(a),
			  control(c)
		{
			ref();
		}

		Shared(std::nullptr_t){}

		Shared(const Shared& other)
		{
			copyFrom(other);
		}

		Shared(Shared&& other)
		{
			moveFrom(std::move(other));
		}

		Shared& operator=(std::nullptr_t)
		{
			unref();
			allocator = nullptr;
			control = nullptr;
			return *this;
		}

		Shared& operator=(Shared&& other)
		{
			unref();
			moveFrom(std::move(other));
			return *this;
		}

		~Shared()
		{
			unref();
		}

		const T& operator*() const
		{
			return *control->ptr;
		}

		T& operator*()
		{
			return *control->ptr;
		}

		const T* operator->() const
		{
			if (control)
				return control->ptr;
			else
				return nullptr;
		}

		T* operator->()
		{
			if (control)
				return control->ptr;
			else
				return nullptr;
		}

		operator bool() const
		{
			return control != nullptr;
		}

		int ref_count() const
		{
			if (control == nullptr)
				return 0;
			return control->strong.load();
		}

		T* get() const
		{
			if (control)
				return control->ptr;
			else
				return nullptr;
		}

		T* leak()
		{
			T* res = nullptr;
			if (control)
				res = control->ptr;
			control = nullptr;
			allocator = nullptr;
			return res;
		}
	};

	template<typename T>
	class Weak
	{
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
			allocator = shared.allocator;
			control = shared.control;
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
						res.allocator = allocator;
						res.control = control;
						return res;
					}

					count = control->strong.load();
				}
			}

			return Shared<T>{};
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
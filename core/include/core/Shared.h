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
	class SharedFromThis;

	template<typename T>
	class Shared
	{
		template<typename>
		friend class Shared;

		template<typename TPtr, typename... TArgs>
		friend inline Shared<TPtr>
		shared_from(Allocator* allocator, TArgs&&... args);

		template<typename>
		friend class Weak;

		template<typename>
		friend class SharedFromThis;

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
					m_allocator->release((void*)m_control->ptr, sizeof(T));
					m_allocator->free((void*)m_control->ptr, sizeof(T));
					m_control->ptr = nullptr;
				}

				if (m_control->weak.fetch_sub(1) == 1)
				{
					m_allocator->release(m_control, sizeof(Control));
					m_allocator->free(m_control, sizeof(Control));
					m_control = nullptr;
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

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		bool constructFromWeak(const Weak<U>& weak);

	public:
		Shared() = default;

		Shared(Allocator* a, Control* c)
			: m_allocator(a),
			  m_control(c)
		{
			ref();
		}

		Shared(std::nullptr_t){}

		Shared(const Shared& other)
		{
			copyFrom(other);
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Shared(const Shared<U>& other)
		{
			copyFrom(other);
		}

		Shared(Shared&& other)
		{
			moveFrom(std::move(other));
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

		Shared& operator=(Shared&& other)
		{
			unref();
			moveFrom(std::move(other));
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

		Shared& operator=(const Shared& other)
		{
			unref();
			copyFrom(std::move(other));
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

		template<typename>
		friend class Shared;

		template<typename>
		friend class Weak;

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

		template<typename U>
		void copyFrom(const Weak<U>& other)
		{
			allocator = other.allocator;
			control = (typename Shared<T>::Control*)other.control;
			ref();
		}

		template<typename U>
		void moveFrom(Weak<U>&& other)
		{
			allocator = other.allocator;
			control = (typename Shared<T>::Control*)other.control;

			other.allocator = nullptr;
			other.control = nullptr;
		}

	public:
		Weak() = default;

		Weak(std::nullptr_t){}

		Weak(const Shared<T>& shared)
		{
			allocator = shared.m_allocator;
			control = (typename Shared<T>::Control*)shared.m_control;
			ref();
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Weak(const Shared<U>& shared)
		{
			allocator = shared.m_allocator;
			control = (typename Shared<T>::Control*)shared.m_control;
			ref();
		}

		Weak(const Weak& other)
		{
			copyFrom(other);
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Weak(const Weak<U>& other)
		{
			copyFrom(other);
		}

		Weak(Weak&& other)
		{
			moveFrom(std::move(other));
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Weak(Weak<U>&& other)
		{
			moveFrom(std::move(other));
		}

		Weak& operator=(std::nullptr_t)
		{
			unref();
			allocator = nullptr;
			control = nullptr;
			return *this;
		}

		Weak& operator=(const Weak& other)
		{
			unref();
			copyFrom(other);
			return *this;
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Weak& operator=(const Weak<U>& other)
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

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Weak& operator=(Weak<U>&& other)
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
			Shared<T> res{};
			res.constructFromWeak(*this);
			return res;
		}
	};

	template<typename T>
	template<typename U>
	requires std::is_convertible_v<U*, T*>
	bool Shared<T>::constructFromWeak(const Weak<U>& weak)
	{
		if (weak.control)
		{
			auto count = weak.control->strong.load();
			while (count != 0)
			{
				if (weak.control->strong.compare_exchange_weak(count, count + 1))
				{
					m_allocator = weak.allocator;
					m_control = (Shared<T>::Control*)weak.control;
					m_control->weak.fetch_add(1);
					return true;
				}

				count = weak.control->strong.load();
			}
		}

		return false;
	}

	template<typename T>
	class SharedFromThis
	{
		template<typename TPtr, typename... TArgs>
		friend inline Shared<TPtr>
		shared_from(Allocator* allocator, TArgs&&... args);

		Weak<T> m_ptr;
	public:
		using SharedFromThisType = SharedFromThis;

		SharedFromThis()
		{
			 int x =234;
		}
		virtual ~SharedFromThis()
		{
			int x = 234;
		}

		[[nodiscard]] Weak<T> weakFromThis() { return m_ptr; }
		[[nodiscard]] Weak<const T> weakFromThis() const { return m_ptr; }
		[[nodiscard]] Shared<T> sharedFromThis()
		{
			Shared<T> res;
			res.constructFromWeak(m_ptr);
			return res;
		}

		[[nodiscard]] Shared<const T> sharedFromThis() const
		{
			Shared<const T> res;
			res.constructFromWeak(m_ptr);
			return res;
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

	template <class T, class = void>
	struct InheritsFromEnableSharedFromThis : std::false_type {};

	template <class T>
	struct InheritsFromEnableSharedFromThis<T, std::void_t<typename T::SharedFromThisType>>
		: std::is_convertible<std::remove_cv_t<T>*, typename T::SharedFromThisType*>::type
	{};

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

		auto res = Shared<T>{allocator, control};

		if constexpr (InheritsFromEnableSharedFromThis<T>::value)
		{
			control->ptr->m_ptr = res;
		}

		return res;
	}
}
#pragma once

#include "core/Allocator.h"
#include "core/HashFunction.h"
#include "core/Assert.h"

#include <atomic>
#include <new>

namespace core
{
	template<typename T>
	class Weak;

	template<typename T>
	class SharedFromThis;

	struct SharedControlBlock
	{
		Allocator* allocator = nullptr;
		Span<std::byte> originalMemory;
		std::atomic<int> strong = 0;
		std::atomic<int> weak = 0;
	};

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

		SharedControlBlock* m_control = nullptr;
		T* m_ptr = nullptr;

		Allocator* getAllocator() const
		{
			if (m_control == nullptr)
				return nullptr;
			return m_control->allocator;
		}

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
				auto allocator = getAllocator();
				if (m_control->strong.fetch_sub(1) == 1)
				{
					m_ptr->~T();
					m_ptr = nullptr;
					allocator->release(m_control->originalMemory);
					allocator->free(m_control->originalMemory);
				}

				if (m_control->weak.fetch_sub(1) == 1)
				{
					allocator->release(Span<std::byte>{(std::byte*)m_control, sizeof(*m_control)});
					allocator->free(Span<std::byte>{(std::byte*)m_control, sizeof(*m_control)});
					m_control = nullptr;
				}
			}
		}

		template<typename U>
		void copyFrom(const Shared<U>& other)
		{
			m_control = other.m_control;
			m_ptr = other.m_ptr;
			ref();
		}

		template<typename U>
		void moveFrom(Shared<U>& other)
		{
			m_control = other.m_control;
			m_ptr = other.m_ptr;
			other.m_control = nullptr;
			other.m_ptr = nullptr;
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		bool constructFromWeak(const Weak<U>& weak);

	public:
		Shared() = default;

		Shared(T* ptr, SharedControlBlock* c)
			: m_control(c),
			  m_ptr(ptr)
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

		Shared(Shared&& other) noexcept
		{
			moveFrom(other);
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Shared(Shared<U>&& other) noexcept
		{
			moveFrom(other);
		}

		Shared& operator=(std::nullptr_t)
		{
			unref();
			m_control = nullptr;
			m_ptr = nullptr;
			return *this;
		}

		Shared& operator=(Shared&& other) noexcept
		{
			unref();
			moveFrom(other);
			return *this;
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Shared& operator=(Shared<U>&& other) noexcept
		{
			unref();
			moveFrom(other);
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

		bool operator==(std::nullptr_t) const { return m_ptr == nullptr; }
		bool operator!=(std::nullptr_t) const { return m_ptr != nullptr; }
		template<typename R>
		bool operator==(const Shared<R>& other) const { return m_ptr == other.m_ptr; }
		template<typename R>
		bool operator!=(const Shared<R>& other) const { return m_ptr != other.m_ptr; }

		int ref_count() const
		{
			if (m_control == nullptr)
				return 0;
			return m_control->strong.load();
		}

		T* get() const
		{
			return m_ptr;
		}

		Allocator* allocator() const { return getAllocator(); }
	};

	template<typename T>
	struct Hash<Shared<T>>
	{
		inline size_t operator()(const Shared<T>& value, size_t seed) const
		{
			return Hash<T*>{}(value.get(), seed);
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

		SharedControlBlock* control = nullptr;
		T* m_ptr = nullptr;

		Allocator* getAllocator()
		{
			if (control == nullptr)
				return nullptr;
			return control->allocator;
		}

		void ref()
		{
			if (control)
				control->weak.fetch_add(1);
		}

		void unref()
		{
			if (control)
			{
				auto allocator = getAllocator();
				if (control->weak.fetch_sub(1) == 1)
				{
					allocator->release(Span<std::byte>{(std::byte*)control, sizeof(*control)});
					allocator->free(Span<std::byte>{(std::byte*)control, sizeof(*control)});
					control = nullptr;
				}
			}
		}

		template<typename U>
		void copyFrom(const Weak<U>& other)
		{
			control = other.control;
			m_ptr = other.m_ptr;
			ref();
		}

		template<typename U>
		void moveFrom(Weak<U>& other)
		{
			control = other.control;
			m_ptr = other.m_ptr;

			other.control = nullptr;
			other.m_ptr = nullptr;
		}

	public:
		Weak() = default;

		Weak(std::nullptr_t){}

		Weak(const Shared<T>& shared)
		{
			control = shared.m_control;
			m_ptr = shared.m_ptr;
			ref();
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Weak(const Shared<U>& shared)
		{
			control = shared.m_control;
			m_ptr = shared.m_ptr;
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

		Weak(Weak&& other) noexcept
		{
			moveFrom(other);
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Weak(Weak<U>&& other) noexcept
		{
			moveFrom(other);
		}

		Weak& operator=(std::nullptr_t)
		{
			unref();
			control = nullptr;
			m_ptr = nullptr;
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

		Weak& operator=(Weak&& other) noexcept
		{
			unref();
			moveFrom(other);
			return *this;
		}

		template<typename U>
		requires std::is_convertible_v<U*, T*>
		Weak& operator=(Weak<U>&& other) noexcept
		{
			unref();
			moveFrom(other);
			return *this;
		}

		~Weak()
		{
			unref();
		}

		operator bool() const
		{
			return m_ptr != nullptr;
		}

		bool operator==(std::nullptr_t) const { return m_ptr == nullptr; }
		bool operator!=(std::nullptr_t) const { return m_ptr != nullptr; }
		template<typename R>
		bool operator==(const Weak<R>& other) const { return m_ptr == other.m_ptr; }
		template<typename R>
		bool operator!=(const Weak<R>& other) const { return m_ptr != other.m_ptr; }

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
					m_ptr = weak.m_ptr;
					m_control = weak.control;
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
		{}
		virtual ~SharedFromThis()
		{}

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
		inline size_t operator()(const Weak<T>& value, size_t seed) const
		{
			return Hash<void*>{}(value.m_ptr, seed);
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
		auto ptr = allocator->allocSingleT<T>();
		allocator->commitSingleT(ptr);

		auto control = allocator->allocSingleT<SharedControlBlock>();
		allocator->commitSingleT(control);

		::new (ptr) T{std::forward<TArgs>(args)...};
		::new (control) SharedControlBlock{};
		control->allocator = allocator;
		control->originalMemory = Span<std::byte>{reinterpret_cast<std::byte*>(ptr), sizeof(*ptr)};

		auto res = Shared<T>{ptr, control};

		if constexpr (InheritsFromEnableSharedFromThis<T>::value)
		{
			ptr->m_ptr = res;
		}

		return res;
	}
}
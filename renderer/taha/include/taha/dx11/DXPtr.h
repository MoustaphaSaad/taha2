#pragma once

#include <type_traits>

#include <d3d11.h>

namespace taha
{
	template <typename T>
		requires std::is_base_of_v<IUnknown, T>
	class DXPtr
	{
		T* m_ptr = nullptr;

	public:
		DXPtr() = default;

		DXPtr(std::nullptr_t) {}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		DXPtr(U* ptr)
			: m_ptr(ptr)
		{}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		DXPtr(DXPtr<U>&& other)
			: m_ptr(other.leak())
		{}

		DXPtr(const DXPtr& other)
			: m_ptr(other.m_ptr)
		{
			if (m_ptr)
			{
				m_ptr->AddRef();
			}
		}

		DXPtr(DXPtr&& other)
			: m_ptr(other.m_ptr)
		{
			other.m_ptr = nullptr;
		}

		~DXPtr()
		{
			if (m_ptr)
			{
				m_ptr->Release();
			}
			m_ptr = nullptr;
		}

		DXPtr& operator=(const DXPtr& other)
		{
			if (m_ptr)
			{
				m_ptr->Release();
			}
			m_ptr = other.m_ptr;
			if (m_ptr)
			{
				m_ptr->AddRef();
			}
			return *this;
		}

		DXPtr& operator=(DXPtr&& other)
		{
			if (m_ptr)
			{
				m_ptr->Release();
			}
			m_ptr = other.leak();
			return *this;
		}

		DXPtr& operator=(std::nullptr_t)
		{
			if (m_ptr)
			{
				m_ptr->Release();
			}
			m_ptr = nullptr;
			return *this;
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
		bool operator==(const DXPtr<R>& other) const
		{
			return m_ptr == other.m_ptr;
		}
		template <typename R>
		bool operator!=(const DXPtr<R>& other) const
		{
			return m_ptr != other.m_ptr;
		}
		T* get()
		{
			return m_ptr;
		}

		[[nodiscard]] T* leak()
		{
			auto p = m_ptr;
			m_ptr = nullptr;
			return p;
		}

		T* const* getPtrAddress() const
		{
			return &m_ptr;
		}
		T** getPtrAddress()
		{
			return &m_ptr;
		}
	};
}
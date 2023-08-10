#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"
#include "core/StringView.h"
#include "core/String.h"

namespace core
{
	class OSString
	{
		Allocator* m_allocator = nullptr;
		char* m_ptr = nullptr;
		size_t m_sizeInBytes = 0;

		void destroy()
		{
			if (m_ptr)
			{
				m_allocator->release(m_ptr, m_sizeInBytes);
				m_allocator->free(m_ptr, m_sizeInBytes);
				m_ptr = nullptr;
				m_sizeInBytes = 0;
			}
		}

		void copyFrom(const OSString& other)
		{
			m_allocator = other.m_allocator;
			m_sizeInBytes = other.m_sizeInBytes;
			m_ptr = (char*)m_allocator->alloc(m_sizeInBytes, alignof(std::max_align_t));
			m_allocator->commit(m_ptr, m_sizeInBytes);
			memcpy(m_ptr, other.m_ptr, m_sizeInBytes);
		}

		void moveFrom(OSString&& other)
		{
			m_allocator = other.m_allocator;
			m_sizeInBytes = other.m_sizeInBytes;
			m_ptr = other.m_ptr;

			other.m_allocator = nullptr;
			other.m_sizeInBytes = 0;
			other.m_ptr = nullptr;
		}

	public:
		CORE_EXPORT explicit OSString(StringView str, Allocator* allocator);

		OSString(const OSString& other)
		{
			copyFrom(other);
		}

		OSString(OSString&& other)
		{
			moveFrom(std::move(other));
		}

		OSString& operator=(const OSString& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		OSString& operator=(OSString&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~OSString()
		{
			destroy();
		}

		const void* data() const
		{
			return m_ptr;
		}

		size_t sizeInBytes() const
		{
			return m_sizeInBytes;
		}

		CORE_EXPORT String toUtf8(Allocator* allocator) const;
	};
}
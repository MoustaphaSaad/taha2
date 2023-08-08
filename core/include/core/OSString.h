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

		CORE_EXPORT void destroy();

		CORE_EXPORT void copyFrom(const OSString& other);

		CORE_EXPORT void moveFrom(OSString&& other);

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
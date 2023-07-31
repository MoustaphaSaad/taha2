#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"
#include "core/StringView.h"
#include "core/String.h"

namespace core
{
	class OSStr
	{
		Allocator* m_allocator = nullptr;
		char* m_ptr = nullptr;
		size_t m_sizeInBytes = 0;

		CORE_EXPORT void destroy();

		CORE_EXPORT void copyFrom(const OSStr& other);

		CORE_EXPORT void moveFrom(OSStr&& other);

	public:
		CORE_EXPORT explicit OSStr(StringView str, Allocator* allocator);

		OSStr(const OSStr& other)
		{
			copyFrom(other);
		}

		OSStr(OSStr&& other)
		{
			moveFrom(std::move(other));
		}

		OSStr& operator=(const OSStr& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		OSStr& operator=(OSStr&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~OSStr()
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
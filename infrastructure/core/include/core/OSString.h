#pragma once

#include "core/Exports.h"
#include "core/Allocator.h"
#include "core/StringView.h"
#include "core/String.h"
#include "core/Buffer.h"

namespace core
{
	class OSString
	{
		Buffer m_buffer;

		explicit OSString(Buffer buffer)
			: m_buffer(std::move(buffer))
		{}

	public:
		static OSString fromOSEncodedBuffer(Buffer buffer)
		{
			return OSString{std::move(buffer)};
		}

		CORE_EXPORT explicit OSString(StringView str, Allocator* allocator);

		const void* data() const
		{
			return m_buffer.data();
		}

		size_t sizeInBytes() const
		{
			return m_buffer.count();
		}

		CORE_EXPORT String toUtf8(Allocator* allocator) const;
	};
}
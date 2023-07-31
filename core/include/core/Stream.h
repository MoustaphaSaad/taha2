#pragma once

#include "core/Exports.h"
#include <cstdint>

namespace core
{
	class Stream
	{
	public:
		CORE_EXPORT virtual ~Stream() = default;
		virtual size_t read(void* buffer, size_t size) = 0;
		virtual size_t write(const void* buffer, size_t size) = 0;
		CORE_EXPORT virtual size_t size() const { return 0; }
		CORE_EXPORT virtual size_t getCursor() const { return 0; }
		CORE_EXPORT virtual size_t moveCursor(int64_t offset) { return 0; }
		CORE_EXPORT virtual size_t setCursor(size_t position) { return 0; }
		CORE_EXPORT virtual size_t moveCursorToStart() { return 0; }
		CORE_EXPORT virtual size_t moveCursorToEnd() { return 0; }
	};
}
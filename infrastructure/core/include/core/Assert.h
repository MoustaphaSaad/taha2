#pragma once

#include "core/Exports.h"

#include <source_location>

#undef assert

namespace core
{
	class Log;

	CORE_EXPORT core::Log* setAssertLog(core::Log* log);

	CORE_EXPORT void assertMsg(bool expr, const char* msg, std::source_location loc = std::source_location::current());

	inline void assertTrue(bool expr, std::source_location loc = std::source_location::current())
	{
		assertMsg(expr, nullptr, loc);
	}

	inline void unreachable(std::source_location loc = std::source_location::current())
	{
		assertMsg(false, "unreachable", loc);
	}

	inline void unreachableMsg(const char* msg, std::source_location loc = std::source_location::current())
	{
		assertMsg(false, msg, loc);
	}
}
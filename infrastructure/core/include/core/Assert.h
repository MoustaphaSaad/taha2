#pragma once

#include "core/Exports.h"

#include <source_location>

namespace core
{
	class Log;

	CORE_EXPORT core::Log* setAssertLog(core::Log* log);

	CORE_EXPORT void
	validateMsg(bool expr, const char* msg, std::source_location loc = std::source_location::current());

	inline void validate(bool expr, std::source_location loc = std::source_location::current())
	{
		validateMsg(expr, nullptr, loc);
	}

	inline void unreachable(std::source_location loc = std::source_location::current())
	{
		validateMsg(false, "unreachable", loc);
	}

	inline void unreachableMsg(const char* msg, std::source_location loc = std::source_location::current())
	{
		validateMsg(false, msg, loc);
	}
}
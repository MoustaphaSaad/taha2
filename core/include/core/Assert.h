#pragma once

#include "core/Exports.h"

#include <source_location>

#if TAHA_COMPILER_MSVC
	#define tahaDebugBreak() __debugbreak()
#elif TAHA_COMPILER_CLANG || TAHA_COMPILER_GNU
	#define tahaDebugBreak() __builtin_trap()
#else
	#error unknown compiler
#endif

namespace core
{
	class Log;

	CORE_EXPORT core::Log* setAssertLog(core::Log* log);
	CORE_EXPORT void __reportAssert(const char* expr, const char* msg, const std::source_location location = std::source_location::current());
}

#ifdef TAHA_ENABLE_ASSERTS
	#define coreAssertMsg(expr, message) do { if (expr) {} else { core::__reportAssert(#expr, message); tahaDebugBreak(); } } while(false)
	#define coreAssert(expr) do { if (expr) {} else { core::__reportAssert(#expr, nullptr); tahaDebugBreak(); } } while(false)
#else
	#define coreAssertMsg(expr, message) ((void)0)
	#define coreAssert(expr) ((void)0)
#endif

#define coreUnreachable() coreAssertMsg(false, "unreachable")
#define coreUnreachableMsg(message) coreAssertMsg(false, message)
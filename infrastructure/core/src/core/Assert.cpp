#include "core/Assert.h"

#include "core/Mallocator.h"
#include "core/Log.h"
#include "core/Stacktrace.h"

namespace core
{
	inline core::Log* defaultAssertLog()
	{
		static Mallocator mallocator;
		static Log log{&mallocator};
		return &log;
	}

	inline core::Log* __ASSERT_LOG = defaultAssertLog();

	core::Log* setAssertLog(core::Log* log)
	{
		auto res = __ASSERT_LOG;
		__ASSERT_LOG = log;
		return res;
	}

	void validateMsg(bool expr, const char* msg, std::source_location loc)
	{
		#ifdef TAHA_ENABLE_ASSERTS
			if (expr)
				return;

			auto log = __ASSERT_LOG;
			auto file = loc.file_name();
			auto function = loc.function_name();
			auto line = loc.line();
			if (msg)
				log->critical("Assertion Failure: {}, message: {}, in file: {}, function: {}, line: {}"_sv, expr, msg, file, function, line);
			else
				log->critical("Assertion Failure: {}, in file: {}, function: {}, line: {}"_sv, expr, file, function, line);

			log->critical("{}"_sv, Stacktrace::current(1, 20).toString(log->allocator(), false));

			#if TAHA_COMPILER_MSVC
				__debugbreak();
			#elif TAHA_COMPILER_CLANG || TAHA_COMPILER_GNU
				__builtin_trap();
			#else
				#error unknown compiler
			#endif
		#endif
	}
}
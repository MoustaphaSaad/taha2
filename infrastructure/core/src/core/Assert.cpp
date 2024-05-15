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

	void __reportAssert(const char* expr, const char* msg, const char* file, const char* function, int line)
	{
		auto log = __ASSERT_LOG;
		if (msg)
			log->critical("Assertion Failure: {}, message: {}, in file: {}, function: {}, line: {}"_sv, expr, msg, file, function, line);
		else
			log->critical("Assertion Failure: {}, in file: {}, function: {}, line: {}"_sv, expr, file, function, line);

		log->critical("{}"_sv, Stacktrace::current(1, 20).toString(log->allocator(), false));
	}
}
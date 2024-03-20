#include <doctest/doctest.h>

#include <core/Stacktrace.h>
#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/File.h>

core::Rawtrace stacktraceBar()
{
	return core::Rawtrace::current(0, 20);
}

core::Rawtrace stacktraceFoo()
{
	return stacktraceBar();
}

TEST_CASE("Stacktrace")
{
	core::Mallocator allocator;
	core::Log log{&allocator};

	auto trace = stacktraceFoo();
	auto stacktrace = trace.resolve();
	log.info("{}"_sv, stacktrace.toString(&allocator, true));

	stacktrace = core::Stacktrace::current(0, 20);
	log.info("{}"_sv, stacktrace.toString(&allocator, true));
}

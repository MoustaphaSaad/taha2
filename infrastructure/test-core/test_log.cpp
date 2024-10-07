#include <doctest/doctest.h>

#include <core/Log.h>
#include <core/Mallocator.h>

TEST_CASE("core::Log basics")
{
	core::Mallocator allocator;

	core::Log logger{&allocator};

	logger.trace("trace message"_sv);
	logger.debug("debug message"_sv);
	logger.info("info message"_sv);
	logger.warn("warn message"_sv);
	logger.error("error message"_sv);
	logger.critical("critical message"_sv);
}
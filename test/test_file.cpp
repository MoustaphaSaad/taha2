#include <doctest/doctest.h>

#include <core/File.h>
#include <core/Mallocator.h>

TEST_CASE("core::File roundtrip")
{
	core::Mallocator allocator;

	auto file = core::File::open(&allocator, "test.txt"_sv, core::File::IO_MODE_READ_WRITE, core::File::OPEN_MODE_CREATE_OVERWRITE);
	REQUIRE(file != nullptr);

	auto str = "Hello يا عالم 🌎!"_sv;
	auto bytesWritten = file->write(str.data(), str.count());
	REQUIRE(bytesWritten == str.count());

	REQUIRE(file->tell() == bytesWritten);
	auto cursor = file->seek(0, core::File::SEEK_MODE_BEGIN);
	REQUIRE(cursor == 0);

	auto buffer_result = core::File::content(&allocator, "test.txt"_sv);
	REQUIRE(buffer_result.isError() == false);
	REQUIRE(str == buffer_result.value());
}

TEST_CASE("core::File print to std files")
{
	auto stdout_msg = "hello from stdout\n"_sv;
	core::File::STDOUT->write(stdout_msg.data(), stdout_msg.count());

	auto stderr_msg = "hello from stderr\n"_sv;
	core::File::STDERR->write(stderr_msg.data(), stderr_msg.count());

	auto stdout_utf8 = "Hello يا عالم 🌎!\n"_sv;
	core::File::STDOUT->write(stdout_utf8.data(), stdout_utf8.count());
}

TEST_CASE("core::File strf to stdout")
{
	core::strf(core::File::STDOUT, "Hello {}!\n"_sv, "يا عالم 🌎");
}

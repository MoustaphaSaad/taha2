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

	char buffer[1024];
	auto bytesRead = file->read(buffer, sizeof(buffer));
	REQUIRE(bytesRead == bytesWritten);
	REQUIRE(memcmp(buffer, str.data(), bytesRead) == 0);
}

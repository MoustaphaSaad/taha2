#include <doctest/doctest.h>

#include <core/Mallocator.h>
#include <core/Msgpack.h>
#include <core/MemoryStream.h>

template<typename T>
inline core::String encode(const T& v, core::Allocator* allocator)
{
	core::MemoryStream stream{allocator};
	auto err = core::msgpack::encode(&stream, v, allocator);
	REQUIRE(!err);

	auto buffer = stream.releaseBuffer();
	core::strf(&stream, "["_sv);
	for (size_t i = 0; i < buffer.count(); ++i)
	{
		if (i > 0)
			core::strf(&stream, ", "_sv);
		core::strf(&stream, "{:#x}"_sv, buffer[i]);
	}
	core::strf(&stream, "]"_sv);
	return stream.releaseString();
}

template<typename T>
inline void decode(std::initializer_list<uint8_t> bytes, T& v, core::Allocator* allocator)
{
	core::MemoryStream stream{allocator};

	auto size = stream.write(bytes.begin(), bytes.size());
	REQUIRE(size == bytes.size());

	stream.seek(0, core::Stream::SEEK_MODE_BEGIN);

	auto err = core::msgpack::decode(&stream, v, allocator);
	REQUIRE(!err);
}

TEST_CASE("msgpack: nil")
{
	core::Mallocator allocator;
	auto bytes = encode(nullptr, &allocator);
	REQUIRE(bytes == "[0xc0]"_sv);
}

TEST_CASE("msgpack: bool")
{
	core::Mallocator allocator;

	auto bytes = encode(true, &allocator);
	REQUIRE(bytes == "[0xc3]"_sv);
	bytes = encode(false, &allocator);
	REQUIRE(bytes == "[0xc2]"_sv);

	bool v;
	decode({0xc3}, v, &allocator);
	REQUIRE(v == true);

	decode({0xc2}, v, &allocator);
	REQUIRE(v == false);
}

TEST_CASE("msgpack: binary")
{
	core::Mallocator allocator;

	core::Buffer buffer{&allocator};
	buffer.push(std::byte(0));
	buffer.push(std::byte(255));

	auto bytes = encode(buffer, &allocator);
	REQUIRE(bytes == "[0xc4, 0x2, 0x0, 0xff]"_sv);

	core::Buffer v{&allocator};
	decode({0xc4, 0x2, 0x0, 0xff}, v, &allocator);
	REQUIRE(v.count() == 2);
	REQUIRE(v[0] == std::byte(0));
	REQUIRE(v[1] == std::byte(255));
}

TEST_CASE("msgpack: numbers")
{
	core::Mallocator allocator;

	auto bytes = encode(0, &allocator);
	REQUIRE(bytes == "[0x0]"_sv);

	bytes = encode(uint64_t(255), &allocator);
	REQUIRE(bytes == "[0xcc, 0xff]"_sv);

	bytes = encode(uint64_t(256), &allocator);
	REQUIRE(bytes == "[0xcd, 0x1, 0x0]"_sv);

	bytes = encode(uint64_t(65535), &allocator);
	REQUIRE(bytes == "[0xcd, 0xff, 0xff]"_sv);

	bytes = encode(uint64_t(65536), &allocator);
	REQUIRE(bytes == "[0xce, 0x0, 0x1, 0x0, 0x0]"_sv);

	bytes = encode(uint64_t(4294967295), &allocator);
	REQUIRE(bytes == "[0xce, 0xff, 0xff, 0xff, 0xff]"_sv);

	bytes = encode(uint64_t(4294967296), &allocator);
	REQUIRE(bytes == "[0xcf, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0]"_sv);

	bytes = encode(uint64_t(18446744073709551615ULL), &allocator);
	REQUIRE(bytes == "[0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff]"_sv);

	bytes = encode(int64_t(0), &allocator);
	REQUIRE(bytes == "[0x0]"_sv);

	bytes = encode(int64_t(127), &allocator);
	REQUIRE(bytes == "[0x7f]"_sv);

	bytes = encode(int64_t(128), &allocator);
	REQUIRE(bytes == "[0xd1, 0x0, 0x80]"_sv);

	bytes = encode(int64_t(32767), &allocator);
	REQUIRE(bytes == "[0xd1, 0x7f, 0xff]"_sv);

	bytes = encode(int64_t(32768), &allocator);
	REQUIRE(bytes == "[0xd2, 0x0, 0x0, 0x80, 0x0]"_sv);

	bytes = encode(int64_t(2147483647), &allocator);
	REQUIRE(bytes == "[0xd2, 0x7f, 0xff, 0xff, 0xff]"_sv);

	bytes = encode(int64_t(2147483648), &allocator);
	REQUIRE(bytes == "[0xd3, 0x0, 0x0, 0x0, 0x0, 0x80, 0x0, 0x0, 0x0]"_sv);

	bytes = encode(int64_t(9223372036854775807LL), &allocator);
	REQUIRE(bytes == "[0xd3, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff]"_sv);

	bytes = encode(int64_t(-32), &allocator);
	REQUIRE(bytes == "[0xe0]"_sv);

	bytes = encode(int64_t(-33), &allocator);
	REQUIRE(bytes == "[0xd0, 0xdf]"_sv);

	bytes = encode(int64_t(-128), &allocator);
	REQUIRE(bytes == "[0xd0, 0x80]"_sv);

	bytes = encode(int64_t(-129), &allocator);
	REQUIRE(bytes == "[0xd1, 0xff, 0x7f]"_sv);

	bytes = encode(int64_t(-32768), &allocator);
	REQUIRE(bytes == "[0xd1, 0x80, 0x0]"_sv);

	bytes = encode(int64_t(-32769), &allocator);
	REQUIRE(bytes == "[0xd2, 0xff, 0xff, 0x7f, 0xff]"_sv);

	bytes = encode(int64_t(-2147483648LL), &allocator);
	REQUIRE(bytes == "[0xd2, 0x80, 0x0, 0x0, 0x0]"_sv);

	bytes = encode(int64_t(-2147483649LL), &allocator);
	REQUIRE(bytes == "[0xd3, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff]"_sv);

	bytes = encode(int64_t(-9223372036854775808LL), &allocator);
	REQUIRE(bytes == "[0xd3, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]"_sv);

	bytes = encode(float(42.42), &allocator);
	REQUIRE(bytes == "[0xca, 0x42, 0x29, 0xae, 0x14]"_sv);

	bytes = encode(double(42.42), &allocator);
	REQUIRE(bytes == "[0xcb, 0x40, 0x45, 0x35, 0xc2, 0x8f, 0x5c, 0x28, 0xf6]"_sv);
}

struct Person
{
	core::String name;
	int age = 0;

	Person(core::Allocator* allocator)
		: name{allocator}
	{}
};

namespace core::msgpack
{
	template<typename TArchive>
	inline HumanError msgpack(TArchive& archive, Person& p)
	{
		return struct_fields(
			archive,
			Field{"name"_sv, p.name},
			Field{"age"_sv, p.age}
		);
	}
}

TEST_CASE("basic msgpack encode/decode")
{
	core::Mallocator allocator;

	Person mostafa{&allocator};
	mostafa.name = "Mostafa"_sv;
	mostafa.age = 30;

	core::MemoryStream stream{&allocator};
	auto err = core::msgpack::encode(&stream, mostafa, &allocator);
	REQUIRE(!err);

	stream.seek(0, core::Stream::SEEK_MODE_BEGIN);

	Person mostafa2{&allocator};
	err = core::msgpack::decode(&stream, mostafa2, &allocator);
	REQUIRE(!err);

	REQUIRE(mostafa.name == mostafa2.name);
	REQUIRE(mostafa.age == mostafa2.age);
}

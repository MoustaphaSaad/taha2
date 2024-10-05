#include <doctest/doctest.h>

#include <core/Mallocator.h>
#include <core/MemoryStream.h>
#include <core/Msgpack.h>

inline bool operator==(const core::Buffer& a, const core::Buffer& b)
{
	if (a.count() != b.count())
	{
		return false;
	}

	for (size_t i = 0; i < a.count(); ++i)
	{
		if (a[i] != b[i])
		{
			return false;
		}
	}

	return true;
}

inline bool operator==(const core::Array<int>& a, const core::Array<int>& b)
{
	if (a.count() != b.count())
	{
		return false;
	}

	for (size_t i = 0; i < a.count(); ++i)
	{
		if (a[i] != b[i])
		{
			return false;
		}
	}

	return true;
}

inline bool operator==(const core::Map<core::String, int>& a, const core::Map<core::String, int>& b)
{
	if (a.count() != b.count())
	{
		return false;
	}

	for (auto& [key, value]: a)
	{
		auto it = b.lookup(key);
		if (it == b.end())
		{
			return false;
		}

		if (it->value != value)
		{
			return false;
		}
	}

	return true;
}

template <typename T>
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
		{
			core::strf(&stream, ", "_sv);
		}
		core::strf(&stream, "{:#x}"_sv, buffer[i]);
	}
	core::strf(&stream, "]"_sv);
	return stream.releaseString();
}

template <typename T>
inline void decode(const T& v, core::Allocator* allocator)
{
	core::MemoryStream stream{allocator};
	auto err = core::msgpack::encode(&stream, v, allocator);
	REQUIRE(!err);

	stream.seek(0, core::Stream::SEEK_MODE_BEGIN);

	auto old = v;
	err = core::msgpack::decode(&stream, old, allocator);
	REQUIRE(!err);

	REQUIRE(old == v);
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

	decode(true, &allocator);
	decode(false, &allocator);
}

TEST_CASE("msgpack: binary")
{
	core::Mallocator allocator;

	core::Buffer buffer{&allocator};
	buffer.push(std::byte(0));
	buffer.push(std::byte(255));

	auto bytes = encode(buffer, &allocator);
	REQUIRE(bytes == "[0xc4, 0x2, 0x0, 0xff]"_sv);

	decode(buffer, &allocator);
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

	bytes = encode(int64_t(INT64_MIN), &allocator);
	REQUIRE(bytes == "[0xd3, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]"_sv);

	bytes = encode(float(42.42), &allocator);
	REQUIRE(bytes == "[0xca, 0x42, 0x29, 0xae, 0x14]"_sv);

	bytes = encode(double(42.42), &allocator);
	REQUIRE(bytes == "[0xcb, 0x40, 0x45, 0x35, 0xc2, 0x8f, 0x5c, 0x28, 0xf6]"_sv);

	decode(uint64_t(0), &allocator);
	decode(uint64_t(255), &allocator);
	decode(uint64_t(256), &allocator);
	decode(uint64_t(65535), &allocator);
	decode(uint64_t(65536), &allocator);
	decode(uint64_t(4294967295), &allocator);
	decode(uint64_t(4294967296), &allocator);
	decode(uint64_t(18446744073709551615ULL), &allocator);
	decode(int64_t(0), &allocator);
	decode(int64_t(127), &allocator);
	decode(int64_t(128), &allocator);
	decode(int64_t(32767), &allocator);
	decode(int64_t(32768), &allocator);
	decode(int64_t(2147483647), &allocator);
	decode(int64_t(2147483648), &allocator);
	decode(int64_t(9223372036854775807LL), &allocator);
	decode(int64_t(-32), &allocator);
	decode(int64_t(-33), &allocator);
	decode(int64_t(-128), &allocator);
	decode(int64_t(-129), &allocator);
	decode(int64_t(-32768), &allocator);
	decode(int64_t(-32769), &allocator);
	decode(int64_t(-2147483648LL), &allocator);
	decode(int64_t(-2147483649LL), &allocator);
	decode(int64_t(INT64_MIN), &allocator);
	decode(float(42.42), &allocator);
	decode(double(42.42), &allocator);
}

TEST_CASE("msgpack: string")
{
	core::Mallocator allocator;

	auto bytes = encode(""_sv, &allocator);
	REQUIRE(bytes == "[0xa0]"_sv);

	bytes = encode("a"_sv, &allocator);
	REQUIRE(bytes == "[0xa1, 0x61]"_sv);

	bytes = encode("1234567890"_sv, &allocator);
	REQUIRE(bytes == "[0xaa, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30]"_sv);

	bytes = encode("1234567890123456789012345678901"_sv, &allocator);
	REQUIRE(
		bytes ==
		"[0xbf, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31]"_sv);

	bytes = encode("12345678901234567890123456789012"_sv, &allocator);
	REQUIRE(
		bytes ==
		"[0xd9, 0x20, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32]"_sv);

	bytes = encode("Кириллица"_sv, &allocator);
	REQUIRE(
		bytes ==
		"[0xb2, 0xd0, 0x9a, 0xd0, 0xb8, 0xd1, 0x80, 0xd0, 0xb8, 0xd0, 0xbb, 0xd0, 0xbb, 0xd0, 0xb8, 0xd1, 0x86, 0xd0, 0xb0]"_sv);

	bytes = encode("ひらがな"_sv, &allocator);
	REQUIRE(bytes == "[0xac, 0xe3, 0x81, 0xb2, 0xe3, 0x82, 0x89, 0xe3, 0x81, 0x8c, 0xe3, 0x81, 0xaa]"_sv);

	bytes = encode("한글"_sv, &allocator);
	REQUIRE(bytes == "[0xa6, 0xed, 0x95, 0x9c, 0xea, 0xb8, 0x80]"_sv);

	bytes = encode("汉字"_sv, &allocator);
	REQUIRE(bytes == "[0xa6, 0xe6, 0xb1, 0x89, 0xe5, 0xad, 0x97]"_sv);

	bytes = encode("مرحبا"_sv, &allocator);
	REQUIRE(bytes == "[0xaa, 0xd9, 0x85, 0xd8, 0xb1, 0xd8, 0xad, 0xd8, 0xa8, 0xd8, 0xa7]"_sv);

	bytes = encode("❤"_sv, &allocator);
	REQUIRE(bytes == "[0xa3, 0xe2, 0x9d, 0xa4]"_sv);

	bytes = encode("🍺"_sv, &allocator);
	REQUIRE(bytes == "[0xa4, 0xf0, 0x9f, 0x8d, 0xba]"_sv);

	decode(core::String{""_sv, &allocator}, &allocator);
	decode(core::String{"a"_sv, &allocator}, &allocator);
	decode(core::String{"1234567890"_sv, &allocator}, &allocator);
	decode(core::String{"1234567890123456789012345678901"_sv, &allocator}, &allocator);
	decode(core::String{"12345678901234567890123456789012"_sv, &allocator}, &allocator);
	decode(core::String{"Кириллица"_sv, &allocator}, &allocator);
	decode(core::String{"ひらがな"_sv, &allocator}, &allocator);
	decode(core::String{"한글"_sv, &allocator}, &allocator);
	decode(core::String{"汉字"_sv, &allocator}, &allocator);
	decode(core::String{"مرحبا"_sv, &allocator}, &allocator);
	decode(core::String{"❤"_sv, &allocator}, &allocator);
	decode(core::String{"🍺"_sv, &allocator}, &allocator);
}

TEST_CASE("msgpack: array")
{
	core::Mallocator allocator;

	core::Array<int> empty{&allocator};
	core::Array<int> simple{&allocator};
	simple.push(1);
	core::Array<int> medium{&allocator};
	for (int i = 0; i < 15; ++i)
	{
		medium.push(i + 1);
	}
	core::Array<int> big{&allocator};
	for (int i = 0; i < 16; ++i)
	{
		big.push(i + 1);
	}

	auto bytes = encode(empty, &allocator);
	REQUIRE(bytes == "[0x90]"_sv);

	bytes = encode(simple, &allocator);
	REQUIRE(bytes == "[0x91, 0x1]"_sv);

	bytes = encode(medium, &allocator);
	REQUIRE(bytes == "[0x9f, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf]"_sv);

	bytes = encode(big, &allocator);
	REQUIRE(
		bytes ==
		"[0xdc, 0x0, 0x10, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10]"_sv);

	decode(empty, &allocator);
	decode(simple, &allocator);
	decode(medium, &allocator);
	decode(big, &allocator);
}

TEST_CASE("msgpack: map")
{
	core::Mallocator allocator;

	core::Map<core::String, int> empty{&allocator};
	core::Map<core::String, int> simple{&allocator};

	simple.insert(core::String{"a"_sv, &allocator}, 1);

	auto bytes = encode(empty, &allocator);
	REQUIRE(bytes == "[0x80]"_sv);

	bytes = encode(simple, &allocator);
	REQUIRE(bytes == "[0x81, 0xa1, 0x61, 0x1]"_sv);

	decode(empty, &allocator);
	decode(simple, &allocator);
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
	template <typename TArchive>
	inline HumanError msgpack(TArchive& archive, Person& p)
	{
		return struct_fields(archive, Field{"name"_sv, p.name}, Field{"age"_sv, p.age});
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

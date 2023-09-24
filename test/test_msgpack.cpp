#include <doctest/doctest.h>

#include <core/Mallocator.h>
#include <core/Msgpack.h>
#include <core/MemoryStream.h>

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

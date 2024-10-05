#include <doctest/doctest.h>

#include <core/Mallocator.h>
#include <core/Unique.h>

TEST_CASE("basic core::Unique test")
{
	core::Mallocator allocator;
	auto ptr = core::unique_from<int>(&allocator, 42);
	REQUIRE(*ptr == 42);
}

class UniqueFoo
{
public:
	virtual ~UniqueFoo() = default;
};

class UniqueBar: public UniqueFoo
{};

class UniqueBaz
{
public:
	operator UniqueFoo() const
	{
		return UniqueFoo{};
	}
};

TEST_CASE("pointer to parent class")
{
	core::Mallocator allocator;
	core::Unique<UniqueFoo> foo;
	auto bar = core::unique_from<UniqueBar>(&allocator);
	foo = std::move(bar);

	// THIS SHOULD ERROR
	// auto baz = core::unique_from<Baz>(&allocator);
	// foo = baz;
}
#include <doctest/doctest.h>

#include <core/Unique.h>
#include <core/Mallocator.h>

TEST_CASE("basic core::Unique test")
{
	core::Mallocator allocator;
	auto ptr = core::unique_from<int>(&allocator, 42);
	REQUIRE(*ptr == 42);
}

class Foo
{
public:
	virtual ~Foo() = default;
};

class Bar: public Foo
{};

class Baz
{
public:
	operator Foo() const { return Foo{}; }
};

TEST_CASE("pointer to parent class")
{
	core::Mallocator allocator;
	core::Unique<Foo> foo;
	auto bar = core::unique_from<Bar>(&allocator);
	foo = std::move(bar);

	// THIS SHOULD ERROR
	// auto baz = core::unique_from<Baz>(&allocator);
	// foo = baz;
}
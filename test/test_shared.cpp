#include <doctest/doctest.h>

#include <core/Shared.h>
#include <core/Mallocator.h>

TEST_CASE("basic core::Unique test")
{
	core::Mallocator allocator;
	core::Shared<int> ptr;
	REQUIRE(ptr.ref_count() == 0);

	ptr = core::shared_from<int>(&allocator, 42);
	REQUIRE(*ptr == 42);
	REQUIRE(ptr.ref_count() == 1);

	core::Weak<int> weak_ptr = ptr;
	REQUIRE(weak_ptr.ref_count() == 1);

	auto ptr2 = weak_ptr.lock();
	REQUIRE(*ptr2 == 42);
	REQUIRE(ptr2.ref_count() == 2);

	ptr = nullptr;
	REQUIRE(ptr2.ref_count() == 1);
	ptr2 = nullptr;
	REQUIRE(weak_ptr.ref_count() == 0);
	REQUIRE(weak_ptr.expired() == true);
}

class Foo: public core::SharedFromThis<Foo>
{
public:
	virtual ~Foo() = default;

	core::Weak<Foo> getWeakPtr() { return weakFromThis(); }
	core::Weak<const Foo> getWeakPtr() const { return weakFromThis(); }
	core::Shared<Foo> getPtr() { return sharedFromThis(); }
	core::Shared<const Foo> getPtr() const { return sharedFromThis(); }
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

	core::Shared<Foo> foo;
	auto bar = core::shared_from<Bar>(&allocator);
	foo = bar;

	static_assert(std::is_convertible_v<Bar*, core::SharedFromThis<Foo>*>);
	auto sharedFoo = foo->getPtr();
	REQUIRE(sharedFoo == foo);

	// std::enable_shared_from_this
	// THIS SHOULD ERROR
	// auto baz = core::shared_from<Baz>(&allocator);
	// foo = baz;
}
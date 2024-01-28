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

class SharedFoo: public core::SharedFromThis<SharedFoo>
{
public:
	virtual ~SharedFoo() = default;

	core::Weak<SharedFoo> getWeakPtr() { return weakFromThis(); }
	core::Weak<const SharedFoo> getWeakPtr() const { return weakFromThis(); }
	core::Shared<SharedFoo> getPtr() { return sharedFromThis(); }
	core::Shared<const SharedFoo> getPtr() const { return sharedFromThis(); }
};

class SharedBar: public SharedFoo
{};

class SharedBaz
{
public:
	operator SharedFoo() const { return SharedFoo{}; }
};

TEST_CASE("pointer to parent class")
{
	core::Mallocator allocator;

	{
		core::Shared<SharedFoo> foo;
		auto bar = core::shared_from<SharedBar>(&allocator);
		foo = bar;

		static_assert(std::is_convertible_v<SharedBar *, core::SharedFromThis<SharedFoo> *>);
		auto sharedFoo = foo->getPtr();
		REQUIRE(sharedFoo == foo);
	}

	// std::enable_shared_from_this
	// THIS SHOULD ERROR
	// auto baz = core::shared_from<Baz>(&allocator);
	// foo = baz;
}
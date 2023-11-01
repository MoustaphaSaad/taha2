#include <doctest/doctest.h>

#include <math/Vector.h>
#include <math/Matrix.h>

TEST_CASE("vector initialization")
{
	SUBCASE("vec2")
	{
		math::vec2<> v{1.0f, 2.0f};
		REQUIRE(v.x == 1.0f);
		REQUIRE(v.y == 2.0f);
	}

	SUBCASE("vec3")
	{
		math::vec3<> v{1.0f, 2.0f, 3.0f};
		REQUIRE(v.x == 1.0f);
		REQUIRE(v.y == 2.0f);
		REQUIRE(v.z == 3.0f);
	}

	SUBCASE("vec4")
	{
		math::vec4<> v{1.0f, 2.0f, 3.0f, 4.0f};
		REQUIRE(v.x == 1.0f);
		REQUIRE(v.y == 2.0f);
		REQUIRE(v.z == 3.0f);
		REQUIRE(v.w == 4.0f);

		math::vec4<math::ModelSpace> v2{1.0f, 2.0f, 3.0f, 4.0f};
		math::mat4<math::ModelSpace, math::WorldSpace> model{};
		auto result = model * v2;
	}
}

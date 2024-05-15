#include <doctest/doctest.h>

#include <math/Vec.h>
#include <math/Mat.h>

TEST_CASE("vector initialization")
{
	SUBCASE("vec2")
	{
		math::float2<> v{1.0f, 2.0f};
		REQUIRE(v.elements.x == 1.0f);
		REQUIRE(v.elements.y == 2.0f);
	}

	SUBCASE("vec3")
	{
		math::float3<> v{1.0f, 2.0f, 3.0f};
		REQUIRE(v.elements.x == 1.0f);
		REQUIRE(v.elements.y == 2.0f);
		REQUIRE(v.elements.z == 3.0f);
	}

	SUBCASE("vec4")
	{
		math::float4<> v{1.0f, 2.0f, 3.0f, 4.0f};
		REQUIRE(v.elements.x == 1.0f);
		REQUIRE(v.elements.y == 2.0f);
		REQUIRE(v.elements.z == 3.0f);
		REQUIRE(v.elements.w == 4.0f);

		math::float4<math::ModelSpace> v2{1.0f, 2.0f, 3.0f, 4.0f};
		math::float4x4<math::ModelSpace, math::WorldSpace> model{};
		auto result = model * v2;
		// TODO: assert result is the same as v2
	}
}

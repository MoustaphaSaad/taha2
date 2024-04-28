#include <math/Vec.h>
#include <math/Mat.h>

#define ANKERL_NANOBENCH_IMPLEMENT 1
#include <nanobench.h>

int main(int argc, char** argv)
{
	ankerl::nanobench::Bench bench{};

	bench.run("math::Mat", [&] {
		math::float4x4<math::ModelSpace, math::WorldSpace> modelMatrix;
		for (size_t i = 0; i < 1000000; ++i)
		{
			math::float4<math::ModelSpace> randVec{float(i), float(i), float(i), float(i)};
			auto outRand = modelMatrix * randVec;
			ankerl::nanobench::doNotOptimizeAway(outRand);
		}
	});

	return EXIT_SUCCESS;
}
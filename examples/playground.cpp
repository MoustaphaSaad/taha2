#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/Stacktrace.h>
#include <core/File.h>

#include <math/Vector.h>
#include <math/Matrix.h>
#include <math/Vec.h>
#include <math/Mat.h>

int main(int argc, char** argv)
{
	if (argc < 2)
		return EXIT_FAILURE;

	auto mode = core::StringView{argv[1]};
	if (mode == "o"_sv)
	{
		math::Matrix<float, 4, math::ModelSpace, math::WorldSpace> modelMatrix;
		for (size_t j = 0; j < 100; ++j)
		{
			for (size_t i = 0; i < 1000000; ++i)
			{
				math::Vector<float, 4, math::ModelSpace> randVec{float(i), float(i), float(i), float(i)};
				auto outRand = modelMatrix * randVec;
				auto equal = outRand[0] == randVec[0] && outRand[1] == randVec[1] && outRand[2] == randVec[2] &&
							 outRand[3] == randVec[3];
				if (equal == false)
					return EXIT_FAILURE;
			}
		}
		return EXIT_SUCCESS;
	}
	else
	{
		math::float4x4 modelMatrix;
		for (size_t j = 0; j < 100; ++j)
		{
			for (size_t i = 0; i < 1000000; ++i)
			{
				math::float4 randVec{float(i), float(i), float(i), float(i)};
				auto outRand = modelMatrix * randVec;
				auto equal = (
						outRand.elements[0] == randVec.elements[0] &&
						outRand.elements[1] == randVec.elements[1] &&
						outRand.elements[2] == randVec.elements[2] &&
						outRand.elements[3] == randVec.elements[3]
				);
				if (equal == false)
					return EXIT_FAILURE;
			}
		}
		return EXIT_SUCCESS;
	}
}
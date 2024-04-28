#pragma once

#include "math/Flags.h"
#include "math/Vec.h"

#include <core/Assert.h>

namespace math
{
	template<typename T, typename V, typename W>
	struct Mat4
	{
		Vec4<T, W> columns[4] = {
			Vec4<T, W>{T(1), T(0), T(0), T(0)},
			Vec4<T, W>{T(0), T(1), T(0), T(0)},
			Vec4<T, W>{T(0), T(0), T(1), T(0)},
			Vec4<T, W>{T(0), T(0), T(0), T(1)}
		};

		constexpr Vec4<T, W>& TAHA_XCALL operator[](size_t index)
		{
			coreAssert(index < 4);
			return columns[index];
		}

		constexpr const Vec4<T, W>& TAHA_XCALL operator[](size_t index) const
		{
			coreAssert(index < 4);
			return columns[index];
		}

		constexpr Vec4<T, W> TAHA_XCALL operator*(Vec4<T, V> other) const
		{
			Vec4<T, W> res;
			res += columns[0] * other.elements[0];
			res += columns[1] * other.elements[1];
			res += columns[2] * other.elements[2];
			res += columns[3] * other.elements[3];
			return res;
		}
	};

	template<typename V = UnknownSpace, typename W = UnknownSpace>
	using float4x4 = Mat4<float, V, W>;
}
#pragma once

#include "math/Flags.h"
#include "math/Vec.h"

#include <core/Assert.h>

namespace math
{
	template<typename T>
	struct Mat4
	{
		Vec4<T> columns[4] = {
			Vec4<T>{T(1), T(0), T(0), T(0)},
			Vec4<T>{T(0), T(1), T(0), T(0)},
			Vec4<T>{T(0), T(0), T(1), T(0)},
			Vec4<T>{T(0), T(0), T(0), T(1)}
		};

		constexpr TAHA_FORCE_INLINE Vec4<T>& TAHA_XCALL operator[](size_t index)
		{
			coreAssert(index < 4);
			return columns[index];
		}

		constexpr TAHA_FORCE_INLINE const Vec4<T>& TAHA_XCALL operator[](size_t index) const
		{
			coreAssert(index < 4);
			return columns[index];
		}

		constexpr TAHA_FORCE_INLINE Vec4<T> TAHA_XCALL operator*(Vec4<T> other) const
		{
			Vec4<T> res;
			res += columns[0] * other.elements[0];
			res += columns[1] * other.elements[1];
			res += columns[2] * other.elements[2];
			res += columns[3] * other.elements[3];
			return res;
		}
	};

	using float4x4 = Mat4<float>;
}
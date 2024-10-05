#pragma once

#include "math/Flags.h"
#include "math/Vec.h"

#include <core/Assert.h>

namespace math
{
	template <typename T, typename V, typename W>
	struct Mat4
	{
		Vec4<T, W> columns[4] = {
			Vec4<T, W>{T(1), T(0), T(0), T(0)},
			Vec4<T, W>{T(0), T(1), T(0), T(0)},
			Vec4<T, W>{T(0), T(0), T(1), T(0)},
			Vec4<T, W>{T(0), T(0), T(0), T(1)}};

		constexpr Mat4() = default;
		constexpr Mat4(T x)
			: columns{Vec4<T, W>{x}, Vec4<T, W>{x}, Vec4<T, W>{x}, Vec4<T, W>{x}}
		{}
		constexpr Mat4(Vec4<T, W> x, Vec4<T, W> y, Vec4<T, W> z, Vec4<T, W> w)
			: columns{x, y, z, w}
		{}

		constexpr Vec4<T, W>& TAHA_XCALL operator[](size_t index)
		{
			core::assertTrue(index < 4);
			return columns[index];
		}

		constexpr const Vec4<T, W>& TAHA_XCALL operator[](size_t index) const
		{
			core::assertTrue(index < 4);
			return columns[index];
		}
	};

	// operator+
	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL operator+(const Mat4<T, V, W>& a, const Mat4<T, V, W>& b)
	{
		return Mat4<T, V, W>{
			a.elements[0] + b.elements[0],
			a.elements[1] + b.elements[1],
			a.elements[2] + b.elements[2],
			a.elements[3] + b.elements[3],
		};
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL operator+(const Mat4<T, V, W>& a, T b)
	{
		return Mat4<T, V, W>{
			a.elements[0] + b,
			a.elements[1] + b,
			a.elements[2] + b,
			a.elements[3] + b,
		};
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL operator+(T b, const Mat4<T, V, W>& a)
	{
		return a + b;
	}

	// operator+=
	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W>& TAHA_XCALL operator+=(Mat4<T, V, W>& a, const Mat4<T, V, W>& b)
	{
		a.elements[0] += b.elements[0];
		a.elements[1] += b.elements[1];
		a.elements[2] += b.elements[2];
		a.elements[3] += b.elements[3];
		return a;
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W>& TAHA_XCALL operator+=(Mat4<T, V, W>& a, T b)
	{
		a.elements[0] += b;
		a.elements[1] += b;
		a.elements[2] += b;
		a.elements[3] += b;
		return a;
	}

	// operator-
	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL operator-(const Mat4<T, V, W>& a, const Mat4<T, V, W>& b)
	{
		return Mat4<T, V, W>{
			a.elements[0] - b.elements[0],
			a.elements[1] - b.elements[1],
			a.elements[2] - b.elements[2],
			a.elements[3] - b.elements[3],
		};
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL operator-(const Mat4<T, V, W>& a, T b)
	{
		return Mat4<T, V, W>{
			a.elements[0] - b,
			a.elements[1] - b,
			a.elements[2] - b,
			a.elements[3] - b,
		};
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL operator-(T b, const Mat4<T, V, W>& a)
	{
		return Mat4<T, V, W>{
			b - a.elements[0],
			b - a.elements[1],
			b - a.elements[2],
			b - a.elements[3],
		};
	}

	// operator-=
	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W>& TAHA_XCALL operator-=(Mat4<T, V, W>& a, const Mat4<T, V, W>& b)
	{
		a.elements[0] -= b.elements[0];
		a.elements[1] -= b.elements[1];
		a.elements[2] -= b.elements[2];
		a.elements[3] -= b.elements[3];
		return a;
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W>& TAHA_XCALL operator-=(Mat4<T, V, W>& a, T b)
	{
		a.elements[0] -= b;
		a.elements[1] -= b;
		a.elements[2] -= b;
		a.elements[3] -= b;
		return a;
	}

	// operator*
	template <typename T, typename V, typename W1, typename W2>
	constexpr Mat4<T, V, W2> TAHA_XCALL operator*(const Mat4<T, W1, W2>& a, const Mat4<T, V, W1>& b)
	{
		auto col0 = (a[0] * b[0][0] + a[1] * b[0][1] + a[2] * b[0][2] + a[3] * b[0][3]);
		auto col1 = (a[0] * b[1][0] + a[1] * b[1][1] + a[2] * b[1][2] + a[3] * b[1][3]);
		auto col2 = (a[0] * b[2][0] + a[1] * b[2][1] + a[2] * b[2][2] + a[3] * b[2][3]);
		auto col3 = (a[0] * b[3][0] + a[1] * b[3][1] + a[2] * b[3][2] + a[3] * b[3][3]);

		return Mat4<T, V, W2>{
			Vec4<T, W2>{col0.elements},
			Vec4<T, W2>{col1.elements},
			Vec4<T, W2>{col2.elements},
			Vec4<T, W2>{col3.elements}};
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL operator*(const Mat4<T, V, W>& a, T b)
	{
		return Mat4<T, V, W>{
			a.columns[0] * b,
			a.columns[1] * b,
			a.columns[2] * b,
			a.columns[3] * b,
		};
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL operator*(T a, const Mat4<T, V, W>& b)
	{
		return b * a;
	}

	template <typename T, typename V, typename W>
	constexpr Vec4<T, W> TAHA_XCALL operator*(const Mat4<T, V, W> a, Vec4<T, V> other)
	{
		Vec4<T, W> res;
		res += a.columns[0] * other.elements[0];
		res += a.columns[1] * other.elements[1];
		res += a.columns[2] * other.elements[2];
		res += a.columns[3] * other.elements[3];
		return res;
	}

	// operator/
	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL operator/(const Mat4<T, V, W>& a, T b)
	{
		return Mat4<T, V, W>{a.columns[0] / b, a.columns[1] / b, a.columns[2] / b, a.columns[3] / b};
	}

	// operator/=
	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W>& TAHA_XCALL operator/=(Mat4<T, V, W>& a, T b)
	{
		a = a / b;
		return a;
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, V, W> TAHA_XCALL transpose(const Mat4<T, V, W>& a)
	{
		return Mat4<T, V, W>{
			Vec4<T, V>{a[0][0], a[1][0], a[2][0], a[3][0]},
			Vec4<T, V>{a[0][1], a[1][1], a[2][1], a[3][1]},
			Vec4<T, V>{a[0][2], a[1][2], a[2][2], a[3][2]},
			Vec4<T, V>{a[0][3], a[1][3], a[2][3], a[3][3]},
		};
	}

	template <typename T, typename V, typename W>
	constexpr T TAHA_XCALL determinant(const Mat4<T, V, W>& a)
	{
		return (a[0][0] * a[1][1] - a[1][0] * a[0][1]) * (a[2][2] * a[3][3] - a[3][2] * a[2][3]) -
			   (a[0][0] * a[2][1] - a[2][0] * a[0][1]) * (a[1][2] * a[3][3] - a[3][2] * a[1][3]) +
			   (a[0][0] * a[3][1] - a[3][0] * a[0][1]) * (a[1][2] * a[2][3] - a[2][2] * a[1][3]) +
			   (a[1][0] * a[2][1] - a[2][0] * a[1][1]) * (a[0][2] * a[3][3] - a[3][2] * a[0][3]) -
			   (a[1][0] * a[3][1] - a[3][0] * a[1][1]) * (a[0][2] * a[2][3] - a[2][2] * a[0][3]) +
			   (a[2][0] * a[3][1] - a[3][0] * a[2][1]) * (a[0][2] * a[1][3] - a[1][2] * a[0][3]);
	}

	template <typename T, typename V, typename W>
	constexpr bool TAHA_XCALL isInvertible(const Mat4<T, V, W>& a)
	{
		return determinant(a) != 0;
	}

	template <typename T, typename V, typename W>
	constexpr Mat4<T, W, V> TAHA_XCALL inverse(const Mat4<T, V, W>& a)
	{
		auto det = determinant(a);
		core::assertTrue(det != 0);

		Mat4<T, W, V> res;
		// 1st col
		res[0][0] =
			(a[1][1] * (a[2][2] * a[3][3] - a[2][3] * a[3][2]) + a[1][2] * (a[2][3] * a[3][1] - a[2][1] * a[3][3]) +
			 a[1][3] * (a[2][1] * a[3][2] - a[2][2] * a[3][1]));

		res[0][1] =
			(a[2][1] * (a[0][2] * a[3][3] - a[0][3] * a[3][2]) + a[2][2] * (a[0][3] * a[3][1] - a[0][1] * a[3][3]) +
			 a[2][3] * (a[0][1] * a[3][2] - a[0][2] * a[3][1]));

		res[0][2] =
			(a[3][1] * (a[0][2] * a[1][3] - a[0][3] * a[1][2]) + a[3][2] * (a[0][3] * a[1][1] - a[0][1] * a[1][3]) +
			 a[3][3] * (a[0][1] * a[1][2] - a[0][2] * a[1][1]));

		res[0][3] =
			(a[0][1] * (a[1][3] * a[2][2] - a[1][2] * a[2][3]) + a[0][2] * (a[1][1] * a[2][3] - a[1][3] * a[2][1]) +
			 a[0][3] * (a[1][2] * a[2][1] - a[1][1] * a[2][2]));

		// 2nd col
		res[1][0] =
			(a[1][2] * (a[2][0] * a[3][3] - a[2][3] * a[3][0]) + a[1][3] * (a[2][2] * a[3][0] - a[2][0] * a[3][2]) +
			 a[1][0] * (a[2][3] * a[3][2] - a[2][2] * a[3][3]));

		res[1][1] =
			(a[2][2] * (a[0][0] * a[3][3] - a[0][3] * a[3][0]) + a[2][3] * (a[0][2] * a[3][0] - a[0][0] * a[3][2]) +
			 a[2][0] * (a[0][3] * a[3][2] - a[0][2] * a[3][3]));

		res[1][2] =
			(a[3][2] * (a[0][0] * a[1][3] - a[0][3] * a[1][0]) + a[3][3] * (a[0][2] * a[1][0] - a[0][0] * a[1][2]) +
			 a[3][0] * (a[0][3] * a[1][2] - a[0][2] * a[1][3]));

		res[1][3] =
			(a[0][2] * (a[1][3] * a[2][0] - a[1][0] * a[2][3]) + a[0][3] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
			 a[0][0] * (a[1][2] * a[2][3] - a[1][3] * a[2][2]));

		// 3rd col
		res[2][0] =
			(a[1][3] * (a[2][0] * a[3][1] - a[2][1] * a[3][0]) + a[1][0] * (a[2][1] * a[3][3] - a[2][3] * a[3][1]) +
			 a[1][1] * (a[2][3] * a[3][0] - a[2][0] * a[3][3]));

		res[2][1] =
			(a[2][3] * (a[0][0] * a[3][1] - a[0][1] * a[3][0]) + a[2][0] * (a[0][1] * a[3][3] - a[0][3] * a[3][1]) +
			 a[2][1] * (a[0][3] * a[3][0] - a[0][0] * a[3][3]));

		res[2][2] =
			(a[3][3] * (a[0][0] * a[1][1] - a[0][1] * a[1][0]) + a[3][0] * (a[0][1] * a[1][3] - a[0][3] * a[1][1]) +
			 a[3][1] * (a[0][3] * a[1][0] - a[0][0] * a[1][3]));

		res[2][3] =
			(a[0][3] * (a[1][1] * a[2][0] - a[1][0] * a[2][1]) + a[0][0] * (a[1][3] * a[2][1] - a[1][1] * a[2][3]) +
			 a[0][1] * (a[1][0] * a[2][3] - a[1][3] * a[2][0]));

		// 4th col
		res[3][0] =
			(a[1][0] * (a[2][2] * a[3][1] - a[2][1] * a[3][2]) + a[1][1] * (a[2][0] * a[3][2] - a[2][2] * a[3][0]) +
			 a[1][2] * (a[2][1] * a[3][0] - a[2][0] * a[3][1]));

		res[3][1] =
			(a[2][0] * (a[0][2] * a[3][1] - a[0][1] * a[3][2]) + a[2][1] * (a[0][0] * a[3][2] - a[0][2] * a[3][0]) +
			 a[2][2] * (a[0][1] * a[3][0] - a[0][0] * a[3][1]));

		res[3][2] =
			(a[3][0] * (a[0][2] * a[1][1] - a[0][1] * a[1][2]) + a[3][1] * (a[0][0] * a[1][2] - a[0][2] * a[1][0]) +
			 a[3][2] * (a[0][1] * a[1][0] - a[0][0] * a[1][1]));

		res[3].w =
			(a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) + a[0][1] * (a[1][2] * a[2][0] - a[1][0] * a[2][2]) +
			 a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]));

		res /= det;
		return res;
	}

	template <typename V = UnknownSpace, typename W = UnknownSpace>
	using float4x4 = Mat4<float, V, W>;
}
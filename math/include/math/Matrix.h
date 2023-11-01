#pragma once

#include "math/Vector.h"

namespace math
{
	template<typename T, size_t N, typename SrcSpace, typename DstSpace>
	struct Matrix
	{
		Vector<T, N, SrcSpace> elements[N];

		Matrix()
		{
			for (size_t i = 0; i < N; ++i)
				elements[i][i] = T{1.0};
		}

		Vector<T, N, SrcSpace>& operator[](size_t index)
		{
			assert(index < N);
			return elements[index];
		}

		const Vector<T, N, SrcSpace>& operator[](size_t index) const
		{
			assert(index < N);
			return elements[index];
		}

		Vector<T, N, DstSpace> operator*(const Vector<T, N, SrcSpace>& other) const
		{
			Vector<T, N, SrcSpace> result{};
			for (size_t i = 0; i < N; ++i)
				result += elements[i] * other[i];
			return castSpace<T, N, SrcSpace, DstSpace>(result);
		}

	private:
		template<typename T, size_t N, typename SrcSpace, typename DstSpace>
		Vector<T, N, DstSpace> castSpace(const Vector<T, N, SrcSpace>& other) const
		{
			Vector<T, N, DstSpace> result{};
			for (size_t i = 0; i < N; ++i)
				result[i] = other[i];
			return result;
		}
	};

	template<typename SrcSpace = UnknownSpace, typename DstSpace = UnknownSpace>
	using mat2 = Matrix<float, 2, SrcSpace, DstSpace>;

	template<typename SrcSpace = UnknownSpace, typename DstSpace = UnknownSpace>
	using mat3 = Matrix<float, 3, SrcSpace, DstSpace>;

	template<typename SrcSpace = UnknownSpace, typename DstSpace = UnknownSpace>
	using mat4 = Matrix<float, 4, SrcSpace, DstSpace>;
}
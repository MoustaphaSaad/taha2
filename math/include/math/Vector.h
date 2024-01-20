#pragma once

#include <cassert>
#include <cstddef>

namespace math
{
	struct UnknownSpace{};
	struct ModelSpace{};
	struct WorldSpace{};
	struct ViewSpace{};

	template<typename T, size_t N, typename VectorSpace>
	struct Vector
	{
		T elements[N];

		T& operator[](size_t index)
		{
			assert(index < N);
			return elements[index];
		}

		const T& operator[](size_t index) const
		{
			assert(index < N);
			return elements[index];
		}

		Vector<T, N, VectorSpace> operator+(const Vector<T, N, VectorSpace>& other) const
		{
			Vector<T, N, VectorSpace> result;
			for (size_t i = 0; i < N; ++i)
				result[i] = elements[i] + other[i];
			return result;
		}

		Vector<T, N, VectorSpace>& operator+=(const Vector<T, N, VectorSpace>& other)
		{
			for (size_t i = 0; i < N; ++i)
				elements[i] += other[i];
			return *this;
		}

		Vector<T, N, VectorSpace> operator*(T scalar) const
		{
			Vector<T, N, VectorSpace> result;
			for (size_t i = 0; i < N; ++i)
				result[i] = elements[i] * scalar;
			return result;
		}

		Vector<T, N, VectorSpace>& operator*=(T scalar)
		{
			for (size_t i = 0; i < N; ++i)
				elements[i] *= scalar;
			return *this;
		}
	};

	template<typename VectorSpace>
	struct Vector<float, 2, VectorSpace>
	{
		union
		{
			struct
			{
				float x, y;
			};
			float elements[2];
		};

		float& operator[](size_t index)
		{
			assert(index < 2);
			return elements[index];
		}

		const float& operator[](size_t index) const
		{
			assert(index < 2);
			return elements[index];
		}

		Vector<float, 2, VectorSpace> operator+(const Vector<float, 2, VectorSpace>& other) const
		{
			Vector<float, 2, VectorSpace> result;
			result[0] = elements[0] + other[0];
			result[1] = elements[1] + other[1];
			return result;
		}

		Vector<float, 2, VectorSpace>& operator+=(const Vector<float, 2, VectorSpace>& other)
		{
			elements[0] += other[0];
			elements[1] += other[1];
			return *this;
		}

		Vector<float, 2, VectorSpace> operator*(float scalar) const
		{
			Vector<float, 2, VectorSpace> result;
			result[0] = elements[0] * scalar;
			result[1] = elements[1] * scalar;
			return result;
		}

		Vector<float, 2, VectorSpace>& operator*=(float scalar)
		{
			elements[0] *= scalar;
			elements[1] *= scalar;
			return *this;
		}
	};

	template<typename VectorSpace>
	struct Vector<float, 3, VectorSpace>
	{
		union
		{
			struct
			{
				float x, y, z;
			};
			float elements[3];
		};

		float& operator[](size_t index)
		{
			assert(index < 3);
			return elements[index];
		}

		const float& operator[](size_t index) const
		{
			assert(index < 3);
			return elements[index];
		}

		Vector<float, 3, VectorSpace> operator+(const Vector<float, 3, VectorSpace>& other) const
		{
			Vector<float, 3, VectorSpace> result;
			result[0] = elements[0] + other[0];
			result[1] = elements[1] + other[1];
			result[2] = elements[2] + other[2];
			return result;
		}

		Vector<float, 3, VectorSpace>& operator+=(const Vector<float, 3, VectorSpace>& other)
		{
			elements[0] += other[0];
			elements[1] += other[1];
			elements[2] += other[2];
			return *this;
		}

		Vector<float, 3, VectorSpace> operator*(float scalar) const
		{
			Vector<float, 3, VectorSpace> result;
			result[0] = elements[0] * scalar;
			result[1] = elements[1] * scalar;
			result[2] = elements[2] * scalar;
			return result;
		}

		Vector<float, 3, VectorSpace>& operator*=(float scalar)
		{
			elements[0] *= scalar;
			elements[1] *= scalar;
			elements[2] *= scalar;
			return *this;
		}
	};

	template<typename VectorSpace>
	struct Vector<float, 4, VectorSpace>
	{
		union
		{
			struct
			{
				float x, y, z, w;
			};
			float elements[4];
		};

		float& operator[](size_t index)
		{
			assert(index < 4);
			return elements[index];
		}

		const float& operator[](size_t index) const
		{
			assert(index < 4);
			return elements[index];
		}

		Vector<float, 4, VectorSpace> operator+(const Vector<float, 4, VectorSpace>& other) const
		{
			Vector<float, 4, VectorSpace> result;
			result[0] = elements[0] + other[0];
			result[1] = elements[1] + other[1];
			result[2] = elements[2] + other[2];
			result[3] = elements[3] + other[3];
			return result;
		}

		Vector<float, 4, VectorSpace>& operator+=(const Vector<float, 4, VectorSpace>& other)
		{
			elements[0] += other[0];
			elements[1] += other[1];
			elements[2] += other[2];
			elements[3] += other[3];
			return *this;
		}

		Vector<float, 4, VectorSpace> operator*(float scalar) const
		{
			Vector<float, 4, VectorSpace> result;
			result[0] = elements[0] * scalar;
			result[1] = elements[1] * scalar;
			result[2] = elements[2] * scalar;
			result[3] = elements[3] * scalar;
			return result;
		}

		Vector<float, 4, VectorSpace>& operator*=(float scalar)
		{
			elements[0] *= scalar;
			elements[1] *= scalar;
			elements[2] *= scalar;
			elements[3] *= scalar;
			return *this;
		}
	};

	template<typename VectorSpace = UnknownSpace>
	using vec2 = Vector<float, 2, VectorSpace>;

	template<typename VectorSpace = UnknownSpace>
	using vec3 = Vector<float, 3, VectorSpace>;

	template<typename VectorSpace = UnknownSpace>
	using vec4 = Vector<float, 4, VectorSpace>;
}
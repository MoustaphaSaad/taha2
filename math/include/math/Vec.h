#pragma once

#include "math/Flags.h"

#include <core/Assert.h>

namespace math
{
	template<typename T, size_t N>
	struct VecStorage
	{
		T elements[N] = {};

		constexpr T& operator[](size_t index)
		{
			coreAssert(index < N);
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			coreAssert(index < N);
			return elements[index];
		}
	};

	template<typename T>
	struct VecStorage<T, 2>
	{
		union
		{
			struct
			{
				T x, y;
			};
			T elements[2];
		};

		constexpr VecStorage()
		{
			for (auto& element: elements)
				element = T(0);
		}

		constexpr VecStorage(T x, T y)
			: elements{x, y}
		{}

		constexpr T& operator[](size_t index)
		{
			coreAssert(index < 2);
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			coreAssert(index < 2);
			return elements[index];
		}
	};

	template<typename T>
	struct VecStorage<T, 3>
	{
		union
		{
			struct
			{
				T x, y, z;
			};
			T elements[3];
		};

		constexpr VecStorage()
		{
			for (auto& element: elements)
				element = T(0);
		}

		constexpr VecStorage(T x, T y, T z)
			: elements{x, y, z}
		{}

		constexpr T& operator[](size_t index)
		{
			coreAssert(index < 3);
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			coreAssert(index < 3);
			return elements[index];
		}
	};

	template<typename T>
	struct VecStorage<T, 4>
	{
		union
		{
			struct
			{
				T x, y, z, w;
			};
			T elements[4];
		};

		constexpr VecStorage()
		{
			for (auto& element: elements)
				element = T(0);
		}

		constexpr VecStorage(T x, T y, T z, T w)
			: elements{x, y, z, w}
		{}

		constexpr T& operator[](size_t index)
		{
			coreAssert(index < 4);
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			coreAssert(index < 4);
			return elements[index];
		}
	};

	struct UnknownSpace{};
	struct ModelSpace{};
	struct WorldSpace{};
	struct Viewspace{};

	template<typename T, typename V>
	struct Vec2
	{
		using VectorSpace = V;
		using Storage = VecStorage<T, 2>;
		Storage elements;

		Vec2() = default;
		constexpr Vec2(Storage e)
			: elements(e)
		{}
		constexpr Vec2(T x, T y)
			: elements{x, y}
		{}

		constexpr Vec2 TAHA_XCALL operator+(Vec2 other) const
		{
			return Vec2{
				elements[0] + other.elements[0],
				elements[1] + other.elements[1]
			};
		}

		constexpr Vec2& TAHA_XCALL operator+=(Vec2 other)
		{
			elements[0] += other.elements[0];
			elements[1] += other.elements[1];
			return *this;
		}

		constexpr Vec2 TAHA_XCALL operator*(T other) const
		{
			return Vec2{
				elements[0] * other,
				elements[1] * other
			};
		}

		constexpr Vec2& TAHA_XCALL operator*=(T other)
		{
			elements[0] *= other;
			elements[1] *= other;
			return *this;
		}
	};

	template<typename T, typename V>
	struct Vec3
	{
		using VectorSpace = V;
		using Storage = VecStorage<T, 3>;
		Storage elements;

		Vec3() = default;
		constexpr Vec3(Storage e)
			: elements(e)
		{}
		constexpr Vec3(T x, T y, T z)
			: elements{x, y, z}
		{}

		constexpr Vec3 TAHA_XCALL operator+(Vec3 other) const
		{
			return Vec3{
				elements[0] + other.elements[0],
				elements[1] + other.elements[1],
				elements[2] + other.elements[2]
			};
		}

		constexpr Vec3& TAHA_XCALL operator+=(Vec3 other)
		{
			elements[0] += other.elements[0];
			elements[1] += other.elements[1];
			elements[2] += other.elements[2];
			return *this;
		}

		constexpr Vec3 TAHA_XCALL operator*(T other) const
		{
			return Vec3{
				elements[0] * other,
				elements[1] * other,
				elements[2] * other
			};
		}

		constexpr Vec3& TAHA_XCALL operator*=(T other)
		{
			elements[0] *= other;
			elements[1] *= other;
			elements[2] *= other;
			return *this;
		}
	};

	template<typename T, typename V>
	struct Vec4
	{
		using VectorSpace = V;
		using Storage = VecStorage<T, 4>;
		Storage elements;

		Vec4() = default;
		constexpr Vec4(Storage e)
			: elements(e)
		{}
		constexpr Vec4(T x, T y, T z, T w)
			: elements{x, y, z, w}
		{}

		constexpr T& operator[](size_t index)
		{
			coreAssert(index < 4);
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			coreAssert(index < 4);
			return elements[index];
		}

		constexpr Vec4 TAHA_XCALL operator+(Vec4 other) const
		{
			return Vec4{
				elements[0] + other.elements[0],
				elements[1] + other.elements[1],
				elements[2] + other.elements[2],
				elements[3] + other.elements[3]
			};
		}

		constexpr Vec4& TAHA_XCALL operator+=(Vec4 other)
		{
			elements[0] += other.elements[0];
			elements[1] += other.elements[1];
			elements[2] += other.elements[2];
			elements[3] += other.elements[3];
			return *this;
		}

		constexpr Vec4 TAHA_XCALL operator*(T other) const
		{
			return Vec4{
				elements[0] * other,
				elements[1] * other,
				elements[2] * other,
				elements[3] * other
			};
		}

		constexpr Vec4& TAHA_XCALL operator*=(T other)
		{
			elements[0] *= other;
			elements[1] *= other;
			elements[2] *= other;
			elements[3] *= other;
			return *this;
		}
	};

	template<typename V = UnknownSpace>
	using float2 = Vec2<float, V>;

	template<typename V = UnknownSpace>
	using float3 = Vec3<float, V>;

	template<typename V = UnknownSpace>
	using float4 = Vec4<float, V>;
}
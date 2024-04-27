#pragma once

#include "math/Flags.h"

#include <core/Assert.h>

namespace math
{
	template<typename T, size_t N>
	struct VecStorage
	{
		T elements[N];

		constexpr TAHA_FORCE_INLINE T& operator[](size_t index)
		{
			coreAssert(index < N);
			return elements[index];
		}

		constexpr TAHA_FORCE_INLINE const T& operator[](size_t index) const
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

		constexpr TAHA_FORCE_INLINE T& operator[](size_t index)
		{
			coreAssert(index < 2);
			return elements[index];
		}

		constexpr TAHA_FORCE_INLINE const T& operator[](size_t index) const
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

		constexpr TAHA_FORCE_INLINE T& operator[](size_t index)
		{
			coreAssert(index < 3);
			return elements[index];
		}

		constexpr TAHA_FORCE_INLINE const T& operator[](size_t index) const
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

		constexpr TAHA_FORCE_INLINE T& operator[](size_t index)
		{
			coreAssert(index < 4);
			return elements[index];
		}

		constexpr TAHA_FORCE_INLINE const T& operator[](size_t index) const
		{
			coreAssert(index < 4);
			return elements[index];
		}
	};

	template<typename T>
	struct Vec2
	{
		using Storage = VecStorage<T, 2>;
		Storage elements = {};

		Vec2() = default;
		TAHA_FORCE_INLINE Vec2(Storage e)
			: elements(e)
		{}
		Vec2(T x, T y)
		{
			elements.x = x;
			elements.y = y;
		}

		constexpr TAHA_FORCE_INLINE Vec2 TAHA_XCALL operator+(Vec2<T> other) const
		{
			Storage res;
			res[0] = elements[0] + other.elements[0];
			res[1] = elements[1] + other.elements[1];
			return res;
		}

		constexpr TAHA_FORCE_INLINE Vec2& TAHA_XCALL operator+=(Vec2 other)
		{
			elements[0] += other.elements[0];
			elements[1] += other.elements[1];
			return *this;
		}

		constexpr TAHA_FORCE_INLINE Vec2 TAHA_XCALL operator*(T other) const
		{
			Storage res;
			res[0] = elements[0] * other;
			res[1] = elements[1] * other;
			return res;
		}

		constexpr TAHA_FORCE_INLINE Vec2& TAHA_XCALL operator*=(T other)
		{
			elements[0] *= other;
			elements[1] *= other;
			return *this;
		}
	};

	template<typename T>
	struct Vec3
	{
		using Storage = VecStorage<T, 3>;
		Storage elements = {};

		Vec3() = default;
		TAHA_FORCE_INLINE Vec3(Storage e)
			: elements(e)
		{}
		Vec3(T x, T y, T z)
		{
			elements.x = x;
			elements.y = y;
			elements.z = z;
		}

		constexpr TAHA_FORCE_INLINE Vec3 TAHA_XCALL operator+(Vec3 other) const
		{
			Storage res;
			res[0] = elements[0] + other.elements[0];
			res[1] = elements[1] + other.elements[1];
			res[2] = elements[2] + other.elements[2];
			return res;
		}

		constexpr TAHA_FORCE_INLINE Vec3& TAHA_XCALL operator+=(Vec3 other)
		{
			elements[0] += other.elements[0];
			elements[1] += other.elements[1];
			elements[2] += other.elements[2];
			return *this;
		}

		constexpr TAHA_FORCE_INLINE Vec3 TAHA_XCALL operator*(T other) const
		{
			Storage res;
			res[0] = elements[0] * other;
			res[1] = elements[1] * other;
			res[2] = elements[2] * other;
			return res;
		}

		constexpr TAHA_FORCE_INLINE Vec3& TAHA_XCALL operator*=(T other)
		{
			elements[0] *= other;
			elements[1] *= other;
			elements[2] *= other;
			return *this;
		}
	};

	template<typename T>
	struct Vec4
	{
		using Storage = VecStorage<T, 4>;
		Storage elements = {};

		Vec4() = default;
		TAHA_FORCE_INLINE Vec4(Storage e)
			: elements(e)
		{}
		TAHA_FORCE_INLINE Vec4(T x, T y, T z, T w)
		{
			elements.x = x;
			elements.y = y;
			elements.z = z;
			elements.w = w;
		}
		Vec4(const Vec4&) = default;
		Vec4(Vec4&&) = default;
		Vec4& operator=(const Vec4&) = default;
		Vec4& operator=(Vec4&&) = default;

		constexpr TAHA_FORCE_INLINE Vec4 TAHA_XCALL operator+(Vec4 other) const
		{
			Storage res;
			res[0] = elements[0] + other.elements[0];
			res[1] = elements[1] + other.elements[1];
			res[2] = elements[2] + other.elements[2];
			res[3] = elements[3] + other.elements[3];
			return res;
		}

		constexpr TAHA_FORCE_INLINE Vec4& TAHA_XCALL operator+=(Vec4 other)
		{
			elements[0] += other.elements[0];
			elements[1] += other.elements[1];
			elements[2] += other.elements[2];
			elements[3] += other.elements[3];
			return *this;
		}

		constexpr TAHA_FORCE_INLINE Vec4 TAHA_XCALL operator*(T other) const
		{
			Storage res;
			res[0] = elements[0] * other;
			res[1] = elements[1] * other;
			res[2] = elements[2] * other;
			res[3] = elements[3] * other;
			return Vec4{res};
		}

		constexpr TAHA_FORCE_INLINE Vec4& TAHA_XCALL operator*=(T other)
		{
			elements[0] *= other;
			elements[1] *= other;
			elements[2] *= other;
			elements[3] *= other;
			return *this;
		}
	};

	using float2 = Vec2<float>;
	using float3 = Vec3<float>;
	using float4 = Vec4<float>;
}
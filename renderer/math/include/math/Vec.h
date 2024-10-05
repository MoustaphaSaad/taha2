#pragma once

#include "math/Flags.h"

#include <core/Assert.h>

#include <cmath>
#include <type_traits>

namespace math
{
	template <typename T, size_t N>
	struct VecStorage
	{
		T elements[N] = {};

		constexpr T& operator[](size_t index)
		{
			core::validate(index < N);
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			core::validate(index < N);
			return elements[index];
		}
	};

	template <typename T>
	struct VecStorage<T, 2>
	{
		union
		{
			struct
			{
				T x, y;
			};
			struct
			{
				T r, g;
			};
			T elements[2];
		};

		constexpr VecStorage()
		{
			for (auto& element: elements)
			{
				element = T(0);
			}
		}

		constexpr VecStorage(T x, T y)
			: elements{x, y}
		{}

		constexpr T& operator[](size_t index)
		{
			core::validate(index < 2);
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			core::validate(index < 2);
			return elements[index];
		}
	};

	template <typename T>
	struct VecStorage<T, 3>
	{
		union
		{
			struct
			{
				T x, y, z;
			};
			struct
			{
				T r, g, b;
			};
			T elements[3];
		};

		constexpr VecStorage()
		{
			for (auto& element: elements)
			{
				element = T(0);
			}
		}

		constexpr VecStorage(T x, T y, T z)
			: elements{x, y, z}
		{}

		constexpr T& operator[](size_t index)
		{
			core::validate(index < 3);
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			core::validate(index < 3);
			return elements[index];
		}
	};

	template <typename T>
	struct VecStorage<T, 4>
	{
		union
		{
			struct
			{
				T x, y, z, w;
			};
			struct
			{
				T r, g, b, a;
			};
			T elements[4];
		};

		constexpr VecStorage()
		{
			for (auto& element: elements)
			{
				element = T(0);
			}
		}

		constexpr VecStorage(T x, T y, T z, T w)
			: elements{x, y, z, w}
		{}

		constexpr T& operator[](size_t index)
		{
			core::validate(index < 4);
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			core::validate(index < 4);
			return elements[index];
		}
	};

	struct UnknownSpace
	{};
	struct ModelSpace
	{};
	struct WorldSpace
	{};
	struct ViewSpace
	{};

	template <typename T, typename V>
	struct Vec2
	{
		using VectorSpace = V;
		using Storage = VecStorage<T, 2>;
		Storage elements;

		Vec2() = default;
		constexpr Vec2(Storage e)
			: elements(e)
		{}
		constexpr Vec2(T x)
			: elements{x, x}
		{}
		constexpr Vec2(T x, T y)
			: elements{x, y}
		{}

		constexpr T& operator[](size_t index)
		{
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			return elements[index];
		}
	};

	// operator+
	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator+(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<T, V>{a.elements[0] + b.elements[0], a.elements[1] + b.elements[1]};
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator+(Vec2<T, V> a, T b)
	{
		return Vec2<T, V>{a.elements[0] + b, a.elements[1] + b};
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator+(T a, Vec2<T, V> b)
	{
		return b + a;
	}

	// operator+=
	template <typename T, typename V>
	constexpr Vec2<T, V>& TAHA_XCALL operator+=(Vec2<T, V>& a, Vec2<T, V> b)
	{
		a.elements[0] += b.elements[0];
		a.elements[1] += b.elements[1];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec2<T, V>& TAHA_XCALL operator+=(Vec2<T, V>& a, T b)
	{
		a.elements[0] += b;
		a.elements[1] += b;
		return a;
	}

	// operator+ unary
	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator+(Vec2<T, V>& a)
	{
		return a;
	}

	// operator- unary
	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator-(Vec2<T, V>& a)
	{
		return Vec2<T, V>{-a.elements[0], -a.elements[1]};
	}

	// operator-
	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator-(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<T, V>{a.elements[0] - b.elements[0], a.elements[1] - b.elements[1]};
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator-(Vec2<T, V> a, T b)
	{
		return Vec2<T, V>{a.elements[0] - b, a.elements[1] - b};
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator-(T a, Vec2<T, V> b)
	{
		return Vec2<T, V>{a - b.elements[0], a - b.elements[1]};
	}

	// operator-=
	template <typename T, typename V>
	constexpr Vec2<T, V>& TAHA_XCALL operator-=(Vec2<T, V>& a, Vec2<T, V> b)
	{
		a.elements[0] -= b.elements[0];
		a.elements[1] -= b.elements[1];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec2<T, V>& TAHA_XCALL operator-=(Vec2<T, V>& a, T b)
	{
		a.elements[0] -= b;
		a.elements[1] -= b;
		return a;
	}

	// operator*
	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator*(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<T, V>{a.elements[0] * b.elements[0], a.elements[1] * b.elements[1]};
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator*(Vec2<T, V> a, T b)
	{
		return Vec2<T, V>{a.elements[0] * b, a.elements[1] * b};
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator*(T a, Vec2<T, V> b)
	{
		return b * a;
	}

	// operator*=
	template <typename T, typename V>
	constexpr Vec2<T, V>& TAHA_XCALL operator*=(Vec2<T, V>& a, Vec2<T, V> b)
	{
		a.elements[0] *= b.elements[0];
		a.elements[1] *= b.elements[1];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec2<T, V>& TAHA_XCALL operator*=(Vec2<T, V>& a, T b)
	{
		a.elements[0] *= b;
		a.elements[1] *= b;
		return a;
	}

	// operator/
	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator/(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<T, V>{a.elements[0] / b.elements[0], a.elements[1] / b.elements[1]};
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator/(Vec2<T, V> a, T b)
	{
		return Vec2<T, V>{a.elements[0] / b, a.elements[1] / b};
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL operator/(T a, Vec2<T, V> b)
	{
		return Vec2<T, V>{a / b.elements[0], a / b.elements[1]};
	}

	// operator/=
	template <typename T, typename V>
	constexpr Vec2<T, V>& TAHA_XCALL operator/=(Vec2<T, V>& a, Vec2<T, V> b)
	{
		a.elements[0] /= b.elements[0];
		a.elements[1] /= b.elements[1];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec2<T, V>& TAHA_XCALL operator/=(Vec2<T, V>& a, T b)
	{
		a.elements[0] /= b;
		a.elements[1] /= b;
		return a;
	}

	// operator==
	template <typename T, typename V>
	constexpr Vec2<bool, UnknownSpace> TAHA_XCALL operator==(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<bool, UnknownSpace>(a.elements[0] == b.elements[0] && a.elements[1] == b.elements[1]);
	}

	// operator!=
	template <typename T, typename V>
	constexpr Vec2<bool, UnknownSpace> TAHA_XCALL operator!=(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<bool, UnknownSpace>(a.elements[0] != b.elements[0] && a.elements[1] != b.elements[1]);
	}

	// operator<
	template <typename T, typename V>
	constexpr Vec2<bool, UnknownSpace> TAHA_XCALL operator<(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<bool, UnknownSpace>(a.elements[0] < b.elements[0] && a.elements[1] < b.elements[1]);
	}

	// operator>
	template <typename T, typename V>
	constexpr Vec2<bool, UnknownSpace> TAHA_XCALL operator>(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<bool, UnknownSpace>(a.elements[0] > b.elements[0] && a.elements[1] > b.elements[1]);
	}

	// operator<=
	template <typename T, typename V>
	constexpr Vec2<bool, UnknownSpace> TAHA_XCALL operator<=(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<bool, UnknownSpace>(a.elements[0] <= b.elements[0] && a.elements[1] <= b.elements[1]);
	}

	// operator>=
	template <typename T, typename V>
	constexpr Vec2<bool, UnknownSpace> TAHA_XCALL operator>=(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<bool, UnknownSpace>(a.elements[0] >= b.elements[0] && a.elements[1] >= b.elements[1]);
	}

	template <typename V>
	constexpr bool TAHA_XCALL all(Vec2<bool, V> a)
	{
		return (a.elements[0] && a.elements[1]);
	}

	template <typename V>
	constexpr bool TAHA_XCALL any(Vec2<bool, V> a)
	{
		return (a.elements[0] || a.elements[1]);
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL mod(Vec2<T, V> a, Vec2<T, V> b)
	{
		if constexpr (std::is_integral_v<T>)
		{
			return Vec2<T, V>{
				a.elements[0] % b.elements[0],
				a.elements[1] % b.elements[1],
			};
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			return Vec2<T, V>{std::fmod(a.elements[0], b.elements[0]), std::fmod(a.elements[1], b.elements[1])};
		}
		else
		{
			static_assert(sizeof(T) == 0, "mod is not defined for this type");
		}
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL mod(Vec2<T, V> a, T b)
	{
		if constexpr (std::is_integral_v<T>)
		{
			return Vec2<T, V>{
				a.elements[0] % b,
				a.elements[1] % b,
			};
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			return Vec2<T, V>{std::fmod(a.elements[0], b), std::fmod(a.elements[1], b)};
		}
		else
		{
			static_assert(sizeof(T) == 0, "mod is not defined for this type");
		}
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL sqrt(Vec2<T, V> a)
	{
		if constexpr (std::is_floating_point_v<T>)
		{
			return Vec2<T, V>{
				std::sqrt(a.elements[0]),
				std::sqrt(a.elements[1]),
			};
		}
		else
		{
			static_assert(sizeof(T) == 0, "mod is not defined for this type");
		}
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL dot(Vec2<T, V> a, Vec2<T, V> b)
	{
		return a.elements[0] * b.elements[0] + a.elements[1] * b.elements[1];
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL min(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<T, V>{
			a.elements[0] < b.elements[0] ? a.elements[0] : b.elements[0],
			a.elements[1] < b.elements[1] ? a.elements[1] : b.elements[1]};
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL min(Vec2<T, V> a)
	{
		return a.elements[0] < a.elements[1] ? a.elements[0] : a.elements[1];
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL max(Vec2<T, V> a, Vec2<T, V> b)
	{
		return Vec2<T, V>{
			a.elements[0] > b.elements[0] ? a.elements[0] : b.elements[0],
			a.elements[1] > b.elements[1] ? a.elements[1] : b.elements[1]};
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL max(Vec2<T, V> a)
	{
		return a.elements[0] > a.elements[1] ? a.elements[0] : a.elements[1];
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL clamp(Vec2<T, V> a, Vec2<T, V> low, Vec2<T, V> high)
	{
		return max(min(a, high), low);
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL clamp(Vec2<T, V> a, T low, T high)
	{
		return max(min(a, high), low);
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL lengthSquared(Vec2<T, V> a)
	{
		return dot(a, a);
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL length(Vec2<T, V> a)
	{
		return std::sqrt(lengthSquared(a));
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL normalize(Vec2<T, V> a)
	{
		return a / length(a);
	}

	template <typename T, typename V>
	constexpr Vec2<T, V> TAHA_XCALL abs(Vec2<T, V> a)
	{
		return Vec2<T, V>{std::abs(a.elements[0]), std::abs(a.elements[1])};
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL sum(Vec2<T, V> a)
	{
		return a.elements[0] + a.elements[1];
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL distance(Vec2<T, V> a, Vec2<T, V> b)
	{
		return length(b - a);
	}

	template <typename T, typename V>
	struct Vec3
	{
		using VectorSpace = V;
		using Storage = VecStorage<T, 3>;
		Storage elements;

		Vec3() = default;
		constexpr Vec3(Storage e)
			: elements(e)
		{}
		constexpr Vec3(T x)
			: elements{x, x, x}
		{}
		constexpr Vec3(T x, T y, T z)
			: elements{x, y, z}
		{}

		constexpr T& operator[](size_t index)
		{
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			return elements[index];
		}
	};

	// operator+
	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator+(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<T, V>{
			a.elements[0] + b.elements[0],
			a.elements[1] + b.elements[1],
			a.elements[2] + b.elements[2],
		};
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator+(Vec3<T, V> a, T b)
	{
		return Vec3<T, V>{a.elements[0] + b, a.elements[1] + b, a.elements[2] + b};
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator+(T a, Vec3<T, V> b)
	{
		return b + a;
	}

	// operator+=
	template <typename T, typename V>
	constexpr Vec3<T, V>& TAHA_XCALL operator+=(Vec3<T, V>& a, Vec3<T, V> b)
	{
		a.elements[0] += b.elements[0];
		a.elements[1] += b.elements[1];
		a.elements[2] += b.elements[2];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec3<T, V>& TAHA_XCALL operator+=(Vec3<T, V>& a, T b)
	{
		a.elements[0] += b;
		a.elements[1] += b;
		a.elements[2] += b;
		return a;
	}

	// operator+ unary
	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator+(Vec3<T, V>& a)
	{
		return a;
	}

	// operator- unary
	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator-(Vec3<T, V>& a)
	{
		return Vec3<T, V>{-a.elements[0], -a.elements[1], -a.elements[2]};
	}

	// operator-
	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator-(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<T, V>{
			a.elements[0] - b.elements[0],
			a.elements[1] - b.elements[1],
			a.elements[2] - b.elements[2],
		};
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator-(Vec3<T, V> a, T b)
	{
		return Vec3<T, V>{a.elements[0] - b, a.elements[1] - b, a.elements[2] - b};
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator-(T a, Vec3<T, V> b)
	{
		return Vec3<T, V>{
			a - b.elements[0],
			a - b.elements[1],
			a - b.elements[2],
		};
	}

	// operator-=
	template <typename T, typename V>
	constexpr Vec3<T, V>& TAHA_XCALL operator-=(Vec3<T, V>& a, Vec3<T, V> b)
	{
		a.elements[0] -= b.elements[0];
		a.elements[1] -= b.elements[1];
		a.elements[2] -= b.elements[2];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec3<T, V>& TAHA_XCALL operator-=(Vec3<T, V>& a, T b)
	{
		a.elements[0] -= b;
		a.elements[1] -= b;
		a.elements[2] -= b;
		return a;
	}

	// operator*
	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator*(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<T, V>{
			a.elements[0] * b.elements[0],
			a.elements[1] * b.elements[1],
			a.elements[2] * b.elements[2],
		};
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator*(Vec3<T, V> a, T b)
	{
		return Vec3<T, V>{
			a.elements[0] * b,
			a.elements[1] * b,
			a.elements[2] * b,
		};
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator*(T a, Vec3<T, V> b)
	{
		return b * a;
	}

	// operator*=
	template <typename T, typename V>
	constexpr Vec3<T, V>& TAHA_XCALL operator*=(Vec3<T, V>& a, Vec3<T, V> b)
	{
		a.elements[0] *= b.elements[0];
		a.elements[1] *= b.elements[1];
		a.elements[2] *= b.elements[2];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec3<T, V>& TAHA_XCALL operator*=(Vec3<T, V>& a, T b)
	{
		a.elements[0] *= b;
		a.elements[1] *= b;
		a.elements[2] *= b;
		return a;
	}

	// operator/
	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator/(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<T, V>{
			a.elements[0] / b.elements[0],
			a.elements[1] / b.elements[1],
			a.elements[2] / b.elements[2],
		};
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator/(Vec3<T, V> a, T b)
	{
		return Vec3<T, V>{
			a.elements[0] / b,
			a.elements[1] / b,
			a.elements[2] / b,
		};
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL operator/(T a, Vec3<T, V> b)
	{
		return Vec3<T, V>{
			a / b.elements[0],
			a / b.elements[1],
			a / b.elements[2],
		};
	}

	// operator/=
	template <typename T, typename V>
	constexpr Vec3<T, V>& TAHA_XCALL operator/=(Vec3<T, V>& a, Vec3<T, V> b)
	{
		a.elements[0] /= b.elements[0];
		a.elements[1] /= b.elements[1];
		a.elements[2] /= b.elements[2];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec3<T, V>& TAHA_XCALL operator/=(Vec3<T, V>& a, T b)
	{
		a.elements[0] /= b;
		a.elements[1] /= b;
		a.elements[2] /= b;
		return a;
	}

	// operator==
	template <typename T, typename V>
	constexpr Vec3<bool, UnknownSpace> TAHA_XCALL operator==(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<bool, UnknownSpace>(
			a.elements[0] == b.elements[0] && a.elements[1] == b.elements[1] && a.elements[2] == b.elements[2]);
	}

	// operator!=
	template <typename T, typename V>
	constexpr Vec3<bool, UnknownSpace> TAHA_XCALL operator!=(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<bool, UnknownSpace>(
			a.elements[0] != b.elements[0] && a.elements[1] != b.elements[1] && a.elements[2] != b.elements[2]);
	}

	// operator<
	template <typename T, typename V>
	constexpr Vec3<bool, UnknownSpace> TAHA_XCALL operator<(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<bool, UnknownSpace>(
			a.elements[0] < b.elements[0] && a.elements[1] < b.elements[1] && a.elements[2] < b.elements[2]);
	}

	// operator>
	template <typename T, typename V>
	constexpr Vec3<bool, UnknownSpace> TAHA_XCALL operator>(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<bool, UnknownSpace>(
			a.elements[0] > b.elements[0] && a.elements[1] > b.elements[1] && a.elements[2] > b.elements[2]);
	}

	// operator<=
	template <typename T, typename V>
	constexpr Vec3<bool, UnknownSpace> TAHA_XCALL operator<=(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<bool, UnknownSpace>(
			a.elements[0] <= b.elements[0] && a.elements[1] <= b.elements[1] && a.elements[2] <= b.elements[2]);
	}

	// operator>=
	template <typename T, typename V>
	constexpr Vec3<bool, UnknownSpace> TAHA_XCALL operator>=(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<bool, UnknownSpace>(
			a.elements[0] >= b.elements[0] && a.elements[1] >= b.elements[1] && a.elements[2] >= b.elements[2]);
	}

	template <typename V>
	constexpr bool TAHA_XCALL all(Vec3<bool, V> a)
	{
		return (a.elements[0] && a.elements[1] && a.elements[2]);
	}

	template <typename V>
	constexpr bool TAHA_XCALL any(Vec3<bool, V> a)
	{
		return (a.elements[0] || a.elements[1] || a.elements[2]);
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL mod(Vec3<T, V> a, Vec3<T, V> b)
	{
		if constexpr (std::is_integral_v<T>)
		{
			return Vec3<T, V>{
				a.elements[0] % b.elements[0],
				a.elements[1] % b.elements[1],
				a.elements[2] % b.elements[2],
			};
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			return Vec3<T, V>{
				std::fmod(a.elements[0], b.elements[0]),
				std::fmod(a.elements[1], b.elements[1]),
				std::fmod(a.elements[2], b.elements[2])};
		}
		else
		{
			static_assert(sizeof(T) == 0, "mod is not defined for this type");
		}
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL mod(Vec3<T, V> a, T b)
	{
		if constexpr (std::is_integral_v<T>)
		{
			return Vec3<T, V>{
				a.elements[0] % b,
				a.elements[1] % b,
				a.elements[2] % b,
			};
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			return Vec3<T, V>{
				std::fmod(a.elements[0], b),
				std::fmod(a.elements[1], b),
				std::fmod(a.elements[2], b),
			};
		}
		else
		{
			static_assert(sizeof(T) == 0, "mod is not defined for this type");
		}
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL sqrt(Vec3<T, V> a)
	{
		if constexpr (std::is_floating_point_v<T>)
		{
			return Vec3<T, V>{
				std::sqrt(a.elements[0]),
				std::sqrt(a.elements[1]),
				std::sqrt(a.elements[2]),
			};
		}
		else
		{
			static_assert(sizeof(T) == 0, "mod is not defined for this type");
		}
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL dot(Vec3<T, V> a, Vec3<T, V> b)
	{
		return a.elements[0] * b.elements[0] + a.elements[1] * b.elements[1] + a.elements[2] * b.elements[2];
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL cross(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<T, V>{
			a.elements[1] * b.elements[2] - a.elements[2] * b.elements[1],
			a.elements[2] * b.elements[0] - a.elements[0] * b.elements[2],
			a.elements[0] * b.elements[1] - a.elements[1] * b.elements[0],
		};
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL min(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<T, V>{
			a.elements[0] < b.elements[0] ? a.elements[0] : b.elements[0],
			a.elements[1] < b.elements[1] ? a.elements[1] : b.elements[1],
			a.elements[2] < b.elements[2] ? a.elements[2] : b.elements[2],
		};
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL min(Vec3<T, V> a)
	{
		auto res = a.elements[0] < a.elements[1] ? a.elements[0] : a.elements[1];
		res = res < a.elements[2] ? res : a.elements[2];
		return res;
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL max(Vec3<T, V> a, Vec3<T, V> b)
	{
		return Vec3<T, V>{
			a.elements[0] > b.elements[0] ? a.elements[0] : b.elements[0],
			a.elements[1] > b.elements[1] ? a.elements[1] : b.elements[1],
			a.elements[2] > b.elements[2] ? a.elements[2] : b.elements[2],
		};
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL max(Vec3<T, V> a)
	{
		auto res = a.elements[0] > a.elements[1] ? a.elements[0] : a.elements[1];
		res = res > a.elements[2] ? res : a.elements[2];
		return res;
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL clamp(Vec3<T, V> a, Vec3<T, V> low, Vec3<T, V> high)
	{
		return max(min(a, high), low);
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL clamp(Vec3<T, V> a, T low, T high)
	{
		return max(min(a, high), low);
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL lengthSquared(Vec3<T, V> a)
	{
		return dot(a, a);
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL length(Vec3<T, V> a)
	{
		return std::sqrt(lengthSquared(a));
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL normalize(Vec3<T, V> a)
	{
		return a / length(a);
	}

	template <typename T, typename V>
	constexpr Vec3<T, V> TAHA_XCALL abs(Vec3<T, V> a)
	{
		return Vec3<T, V>{
			std::abs(a.elements[0]),
			std::abs(a.elements[1]),
			std::abs(a.elements[2]),
		};
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL sum(Vec3<T, V> a)
	{
		return a.elements[0] + a.elements[1] + a.elements[2];
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL distance(Vec3<T, V> a, Vec3<T, V> b)
	{
		return length(b - a);
	}

	template <typename T, typename V>
	struct Vec4
	{
		using VectorSpace = V;
		using Storage = VecStorage<T, 4>;
		Storage elements;

		Vec4() = default;
		constexpr Vec4(Storage e)
			: elements(e)
		{}
		constexpr Vec4(T x)
			: elements{x, x, x, x}
		{}
		constexpr Vec4(T x, T y, T z, T w)
			: elements{x, y, z, w}
		{}

		constexpr T& operator[](size_t index)
		{
			return elements[index];
		}

		constexpr const T& operator[](size_t index) const
		{
			return elements[index];
		}
	};

	// operator+
	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator+(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<T, V>{
			a.elements[0] + b.elements[0],
			a.elements[1] + b.elements[1],
			a.elements[2] + b.elements[2],
			a.elements[3] + b.elements[3],
		};
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator+(Vec4<T, V> a, T b)
	{
		return Vec4<T, V>{
			a.elements[0] + b,
			a.elements[1] + b,
			a.elements[2] + b,
			a.elements[3] + b,
		};
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator+(T a, Vec4<T, V> b)
	{
		return b + a;
	}

	// operator+=
	template <typename T, typename V>
	constexpr Vec4<T, V>& TAHA_XCALL operator+=(Vec4<T, V>& a, Vec4<T, V> b)
	{
		a.elements[0] += b.elements[0];
		a.elements[1] += b.elements[1];
		a.elements[2] += b.elements[2];
		a.elements[3] += b.elements[3];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec4<T, V>& TAHA_XCALL operator+=(Vec4<T, V>& a, T b)
	{
		a.elements[0] += b;
		a.elements[1] += b;
		a.elements[2] += b;
		a.elements[3] += b;
		return a;
	}

	// operator+ unary
	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator+(Vec4<T, V>& a)
	{
		return a;
	}

	// operator- unary
	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator-(Vec4<T, V>& a)
	{
		return Vec4<T, V>{
			-a.elements[0],
			-a.elements[1],
			-a.elements[2],
			-a.elements[3],
		};
	}

	// operator-
	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator-(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<T, V>{
			a.elements[0] - b.elements[0],
			a.elements[1] - b.elements[1],
			a.elements[2] - b.elements[2],
			a.elements[3] - b.elements[3],
		};
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator-(Vec4<T, V> a, T b)
	{
		return Vec4<T, V>{
			a.elements[0] - b,
			a.elements[1] - b,
			a.elements[2] - b,
			a.elements[3] - b,
		};
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator-(T a, Vec4<T, V> b)
	{
		return Vec4<T, V>{
			a - b.elements[0],
			a - b.elements[1],
			a - b.elements[2],
			a - b.elements[3],
		};
	}

	// operator-=
	template <typename T, typename V>
	constexpr Vec4<T, V>& TAHA_XCALL operator-=(Vec4<T, V>& a, Vec4<T, V> b)
	{
		a.elements[0] -= b.elements[0];
		a.elements[1] -= b.elements[1];
		a.elements[2] -= b.elements[2];
		a.elements[3] -= b.elements[3];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec4<T, V>& TAHA_XCALL operator-=(Vec4<T, V>& a, T b)
	{
		a.elements[0] -= b;
		a.elements[1] -= b;
		a.elements[2] -= b;
		a.elements[3] -= b;
		return a;
	}

	// operator*
	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator*(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<T, V>{
			a.elements[0] * b.elements[0],
			a.elements[1] * b.elements[1],
			a.elements[2] * b.elements[2],
			a.elements[3] * b.elements[3],
		};
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator*(Vec4<T, V> a, T b)
	{
		return Vec4<T, V>{
			a.elements[0] * b,
			a.elements[1] * b,
			a.elements[2] * b,
			a.elements[3] * b,
		};
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator*(T a, Vec4<T, V> b)
	{
		return b * a;
	}

	// operator*=
	template <typename T, typename V>
	constexpr Vec4<T, V>& TAHA_XCALL operator*=(Vec4<T, V>& a, Vec4<T, V> b)
	{
		a.elements[0] *= b.elements[0];
		a.elements[1] *= b.elements[1];
		a.elements[2] *= b.elements[2];
		a.elements[3] *= b.elements[3];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec4<T, V>& TAHA_XCALL operator*=(Vec4<T, V>& a, T b)
	{
		a.elements[0] *= b;
		a.elements[1] *= b;
		a.elements[2] *= b;
		a.elements[3] *= b;
		return a;
	}

	// operator/
	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator/(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<T, V>{
			a.elements[0] / b.elements[0],
			a.elements[1] / b.elements[1],
			a.elements[2] / b.elements[2],
			a.elements[3] / b.elements[3],
		};
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator/(Vec4<T, V> a, T b)
	{
		return Vec4<T, V>{
			a.elements[0] / b,
			a.elements[1] / b,
			a.elements[2] / b,
			a.elements[3] / b,
		};
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL operator/(T a, Vec4<T, V> b)
	{
		return Vec4<T, V>{
			a / b.elements[0],
			a / b.elements[1],
			a / b.elements[2],
			a / b.elements[3],
		};
	}

	// operator/=
	template <typename T, typename V>
	constexpr Vec4<T, V>& TAHA_XCALL operator/=(Vec4<T, V>& a, Vec4<T, V> b)
	{
		a.elements[0] /= b.elements[0];
		a.elements[1] /= b.elements[1];
		a.elements[2] /= b.elements[2];
		a.elements[3] /= b.elements[3];
		return a;
	}

	template <typename T, typename V>
	constexpr Vec4<T, V>& TAHA_XCALL operator/=(Vec4<T, V>& a, T b)
	{
		a.elements[0] /= b;
		a.elements[1] /= b;
		a.elements[2] /= b;
		a.elements[3] /= b;
		return a;
	}

	// operator==
	template <typename T, typename V>
	constexpr Vec4<bool, UnknownSpace> TAHA_XCALL operator==(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<bool, UnknownSpace>(
			a.elements[0] == b.elements[0] && a.elements[1] == b.elements[1] && a.elements[2] == b.elements[2] &&
			a.elements[3] == b.elements[3]);
	}

	// operator!=
	template <typename T, typename V>
	constexpr Vec4<bool, UnknownSpace> TAHA_XCALL operator!=(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<bool, UnknownSpace>(
			a.elements[0] != b.elements[0] && a.elements[1] != b.elements[1] && a.elements[2] != b.elements[2] &&
			a.elements[3] != b.elements[3]);
	}

	// operator<
	template <typename T, typename V>
	constexpr Vec4<bool, UnknownSpace> TAHA_XCALL operator<(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<bool, UnknownSpace>(
			a.elements[0] < b.elements[0] && a.elements[1] < b.elements[1] && a.elements[2] < b.elements[2] &&
			a.elements[3] < b.elements[3]);
	}

	// operator>
	template <typename T, typename V>
	constexpr Vec4<bool, UnknownSpace> TAHA_XCALL operator>(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<bool, UnknownSpace>(
			a.elements[0] > b.elements[0] && a.elements[1] > b.elements[1] && a.elements[2] > b.elements[2] &&
			a.elements[3] > b.elements[3]);
	}

	// operator<=
	template <typename T, typename V>
	constexpr Vec4<bool, UnknownSpace> TAHA_XCALL operator<=(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<bool, UnknownSpace>(
			a.elements[0] <= b.elements[0] && a.elements[1] <= b.elements[1] && a.elements[2] <= b.elements[2] &&
			a.elements[3] <= b.elements[3]);
	}

	// operator>=
	template <typename T, typename V>
	constexpr Vec4<bool, UnknownSpace> TAHA_XCALL operator>=(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<bool, UnknownSpace>(
			a.elements[0] >= b.elements[0] && a.elements[1] >= b.elements[1] && a.elements[2] >= b.elements[2] &&
			a.elements[3] >= b.elements[3]);
	}

	template <typename V>
	constexpr bool TAHA_XCALL all(Vec4<bool, V> a)
	{
		return (a.elements[0] && a.elements[1] && a.elements[2] && a.elements[3]);
	}

	template <typename V>
	constexpr bool TAHA_XCALL any(Vec4<bool, V> a)
	{
		return (a.elements[0] || a.elements[1] || a.elements[2] || a.elements[3]);
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL mod(Vec4<T, V> a, Vec4<T, V> b)
	{
		if constexpr (std::is_integral_v<T>)
		{
			return Vec4<T, V>{
				a.elements[0] % b.elements[0],
				a.elements[1] % b.elements[1],
				a.elements[2] % b.elements[2],
				a.elements[3] % b.elements[3],
			};
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			return Vec4<T, V>{
				std::fmod(a.elements[0], b.elements[0]),
				std::fmod(a.elements[1], b.elements[1]),
				std::fmod(a.elements[2], b.elements[2]),
				std::fmod(a.elements[3], b.elements[3]),
			};
		}
		else
		{
			static_assert(sizeof(T) == 0, "mod is not defined for this type");
		}
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL mod(Vec4<T, V> a, T b)
	{
		if constexpr (std::is_integral_v<T>)
		{
			return Vec4<T, V>{
				a.elements[0] % b,
				a.elements[1] % b,
				a.elements[2] % b,
				a.elements[3] % b,
			};
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			return Vec4<T, V>{
				std::fmod(a.elements[0], b),
				std::fmod(a.elements[1], b),
				std::fmod(a.elements[2], b),
				std::fmod(a.elements[3], b),
			};
		}
		else
		{
			static_assert(sizeof(T) == 0, "mod is not defined for this type");
		}
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL sqrt(Vec4<T, V> a)
	{
		if constexpr (std::is_floating_point_v<T>)
		{
			return Vec4<T, V>{
				std::sqrt(a.elements[0]),
				std::sqrt(a.elements[1]),
				std::sqrt(a.elements[2]),
				std::sqrt(a.elements[3]),
			};
		}
		else
		{
			static_assert(sizeof(T) == 0, "mod is not defined for this type");
		}
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL dot(Vec4<T, V> a, Vec4<T, V> b)
	{
		return a.elements[0] * b.elements[0] + a.elements[1] * b.elements[1] + a.elements[2] * b.elements[2] +
			   a.elements[3] * b.elements[3];
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL min(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<T, V>{
			a.elements[0] < b.elements[0] ? a.elements[0] : b.elements[0],
			a.elements[1] < b.elements[1] ? a.elements[1] : b.elements[1],
			a.elements[2] < b.elements[2] ? a.elements[2] : b.elements[2],
			a.elements[3] < b.elements[3] ? a.elements[3] : b.elements[3],
		};
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL min(Vec4<T, V> a)
	{
		auto res = a.elements[0] < a.elements[1] ? a.elements[0] : a.elements[1];
		res = res < a.elements[2] ? res : a.elements[2];
		res = res < a.elements[3] ? res : a.elements[3];
		return res;
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL max(Vec4<T, V> a, Vec4<T, V> b)
	{
		return Vec4<T, V>{
			a.elements[0] > b.elements[0] ? a.elements[0] : b.elements[0],
			a.elements[1] > b.elements[1] ? a.elements[1] : b.elements[1],
			a.elements[2] > b.elements[2] ? a.elements[2] : b.elements[2],
			a.elements[3] > b.elements[3] ? a.elements[3] : b.elements[3],
		};
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL max(Vec4<T, V> a)
	{
		auto res = a.elements[0] > a.elements[1] ? a.elements[0] : a.elements[1];
		res = res > a.elements[2] ? res : a.elements[2];
		res = res > a.elements[3] ? res : a.elements[3];
		return res;
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL clamp(Vec4<T, V> a, Vec4<T, V> low, Vec4<T, V> high)
	{
		return max(min(a, high), low);
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL clamp(Vec4<T, V> a, T low, T high)
	{
		return max(min(a, high), low);
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL lengthSquared(Vec4<T, V> a)
	{
		return dot(a, a);
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL length(Vec4<T, V> a)
	{
		return std::sqrt(lengthSquared(a));
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL normalize(Vec4<T, V> a)
	{
		return a / length(a);
	}

	template <typename T, typename V>
	constexpr Vec4<T, V> TAHA_XCALL abs(Vec4<T, V> a)
	{
		return Vec4<T, V>{
			std::abs(a.elements[0]),
			std::abs(a.elements[1]),
			std::abs(a.elements[2]),
			std::abs(a.elements[3]),
		};
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL sum(Vec4<T, V> a)
	{
		return a.elements[0] + a.elements[1] + a.elements[2] + a.elements[3];
	}

	template <typename T, typename V>
	constexpr T TAHA_XCALL distance(Vec4<T, V> a, Vec4<T, V> b)
	{
		return length(b - a);
	}

	template <typename V = UnknownSpace>
	using float2 = Vec2<float, V>;

	template <typename V = UnknownSpace>
	using float3 = Vec3<float, V>;

	template <typename V = UnknownSpace>
	using float4 = Vec4<float, V>;

	using rgb = Vec3<float, UnknownSpace>;
	using rgba = Vec4<float, UnknownSpace>;
}
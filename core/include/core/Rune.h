#pragma once

#include "core/Exports.h"

#include <cstddef>

namespace core
{
	class Rune
	{
		int m_value = 0;
	public:
		Rune() = default;
		explicit Rune(int value)
			: m_value(value)
		{}
		operator int() const { return m_value; }

		bool operator==(const Rune& other) const { return m_value == other.m_value; }
		bool operator!=(const Rune& other) const { return m_value != other.m_value; }
		bool operator<(const Rune& other) const { return m_value < other.m_value; }
		bool operator>(const Rune& other) const { return m_value > other.m_value; }
		bool operator<=(const Rune& other) const { return m_value <= other.m_value; }
		bool operator>=(const Rune& other) const { return m_value >= other.m_value; }

		CORE_EXPORT Rune lower() const;
		CORE_EXPORT Rune upper() const;
		CORE_EXPORT size_t charWidth() const;
		CORE_EXPORT bool isLetter() const;
		CORE_EXPORT bool isNumber() const;
		CORE_EXPORT bool isValid() const;

		CORE_EXPORT static size_t count(const char* ptr);
		CORE_EXPORT static size_t count(const char* begin, const char* end);
		CORE_EXPORT static const char* next(const char* ptr);
		CORE_EXPORT static const char* prev(const char* ptr);
		CORE_EXPORT static Rune decode(const char* ptr);
		CORE_EXPORT static size_t encode(Rune c, char* ptr);
	};
}
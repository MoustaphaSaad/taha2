#pragma once

#include "core/Exports.h"

#include <cstddef>

namespace core
{
	class CORE_EXPORT Rune
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

		Rune lower() const;
		Rune upper() const;
		size_t charWidth() const;
		bool isLetter() const;
		bool isNumber() const;
		bool isValid() const;

		static size_t count(const char* ptr);
		static const char* next(const char* ptr);
		static const char* prev(const char* ptr);
		static Rune decode(const char* ptr);
		static size_t encode(Rune c, char* ptr);
	};
}
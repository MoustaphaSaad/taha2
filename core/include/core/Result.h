#pragma once

#include "core/String.h"
#include "core/StringView.h"

#include <utility>
#include <cassert>

namespace core
{
	// error message for humans
	class [[nodiscard]] HumanError
	{
		core::String m_message;
	public:
		explicit HumanError(core::String message) : m_message(std::move(message)) {}

		core::StringView message() const { return m_message; }

		operator bool() const { return m_message.count() > 0; }
	};

	template<typename T, typename E = HumanError>
	class Result
	{
		enum STATE
		{
			STATE_EMPTY,
			STATE_VALUE,
			STATE_ERROR,
		};

		static constexpr auto SIZE = sizeof(T) > sizeof(E) ? sizeof(T) : sizeof(E);
		static constexpr auto ALIGNMENT = alignof(T) > alignof(E) ? alignof(T) : alignof(E);
		alignas(ALIGNMENT) unsigned char m_storage[SIZE];
		STATE m_state = STATE_EMPTY;
	public:

		template<typename U>
		Result(U&& value)
		{
			::new (m_storage) T(std::forward<U>(value));
			m_state = STATE_VALUE;
		}

		Result(E&& error)
		{
			::new (m_storage) E(std::move(error));
			m_state = STATE_ERROR;
		}

		Result(const E&& error)
		{
			::new (m_storage) E(std::move(error));
			m_state = STATE_ERROR;
		}

		bool is_error() const { return m_state == STATE_ERROR; }

		T& value()
		{
			assert(m_state == STATE_VALUE);
			return *reinterpret_cast<T*>(m_storage);
		}
		const T& value() const
		{
			assert(m_state == STATE_VALUE);
			return *reinterpret_cast<T*>(m_storage);
		}

		E& error()
		{
			assert(m_state == STATE_ERROR);
			return *reinterpret_cast<E*>(m_storage);
		}
		const E& error() const
		{
			assert(m_state == STATE_ERROR);
			return *reinterpret_cast<E*>(m_storage);
		}

		T release_value()
		{
			assert(m_state == STATE_VALUE);
			m_state = STATE_EMPTY;
			return std::move(*reinterpret_cast<T*>(m_storage));
		}
		E release_error()
		{
			assert(m_state == STATE_ERROR);
			m_state = STATE_EMPTY;
			return std::move(*reinterpret_cast<E*>(m_storage));
		}
	};

	template<typename ... Args>
	inline HumanError errf(Allocator* allocator, StringView format, Args&& ... args)
	{
		return HumanError{strf(allocator, format, std::forward<Args>(args)...)};
	}
}
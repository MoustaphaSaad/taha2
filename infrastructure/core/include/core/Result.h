#pragma once

#include "core/String.h"
#include "core/StringView.h"
#include "core/Assert.h"

#include <utility>

#include <fmt/core.h>

namespace core
{
	// error message for humans
	class [[nodiscard]] HumanError
	{
		String m_message;
	public:
		HumanError()
			: m_message(nullptr)
		{}

		explicit HumanError(String message) : m_message(std::move(message)) {}

		StringView message() const { return m_message; }

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

		void destroy()
		{
			switch(m_state)
			{
			case STATE_EMPTY:
				break;
			case STATE_VALUE:
				((T*)m_storage)->~T();
				break;
			case STATE_ERROR:
				((E*)m_storage)->~E();
				break;
			default:
				unreachable();
				break;
			}
			m_state = STATE_EMPTY;
		}

		void copyFrom(const Result& other)
		{
			m_state = other.m_state;
			switch (m_state)
			{
			case STATE_EMPTY:
				break;
			case STATE_VALUE:
				::new (m_storage) T(*(T*)other.m_storage);
				break;
			case STATE_ERROR:
				::new (m_storage) E(*(E*)other.m_storage);
				break;
			default:
				unreachable();
				break;
			}
		}

		void moveFrom(Result& other)
		{
			m_state = other.m_state;
			switch (m_state)
			{
			case STATE_EMPTY:
				break;
			case STATE_VALUE:
				::new (m_storage) T(std::move(*(T*)other.m_storage));
				break;
			case STATE_ERROR:
				::new (m_storage) E(std::move(*(E*)other.m_storage));
				break;
			default:
				unreachable();
				break;
			}
			other.m_state = STATE_EMPTY;
		}

		Result() = default;
	public:

		static Result createEmpty() { return Result{}; }

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

		Result(const Result& value)
		{
			copyFrom(value);
		}

		Result(Result&& value) noexcept
		{
			moveFrom(value);
		}

		Result& operator=(const Result& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		Result& operator=(Result&& other) noexcept
		{
			destroy();
			moveFrom(other);
			return *this;
		}

		~Result()
		{
			destroy();
		}

		bool isError() const { return m_state == STATE_ERROR; }
		bool isValue() const { return m_state == STATE_VALUE; }
		bool isEmpty() const { return m_state == STATE_EMPTY; }

		T& value()
		{
			validate(m_state == STATE_VALUE);
			return *reinterpret_cast<T*>(m_storage);
		}
		const T& value() const
		{
			validate(m_state == STATE_VALUE);
			return *reinterpret_cast<T*>(m_storage);
		}

		E& error()
		{
			validate(m_state == STATE_ERROR);
			return *reinterpret_cast<E*>(m_storage);
		}
		const E& error() const
		{
			validate(m_state == STATE_ERROR);
			return *reinterpret_cast<E*>(m_storage);
		}

		T releaseValue()
		{
			validate(m_state == STATE_VALUE);
			m_state = STATE_EMPTY;
			return std::move(*reinterpret_cast<T*>(m_storage));
		}
		E releaseError()
		{
			validate(m_state == STATE_ERROR);
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

namespace fmt
{
	template<>
	struct formatter<core::HumanError>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx)
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format(const core::HumanError& err, FormatContext& ctx)
		{
			return format_to(ctx.out(), fmt::runtime("{}"), err.message());
		}
	};
}
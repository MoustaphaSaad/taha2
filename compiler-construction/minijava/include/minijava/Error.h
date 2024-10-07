#pragma once

#include "minijava/Token.h"

#include <core/String.h>

namespace minijava
{
	class Error
	{
		Location m_location;
		core::String m_message;

	public:
		Error(Location location, core::String message)
			: m_location(location),
			  m_message(std::move(message))
		{}

		Location location() const
		{
			return m_location;
		}
		core::StringView message() const
		{
			return m_message;
		}
	};

	template <typename... Args>
	[[nodiscard]] inline Error
	errf(core::Allocator* allocator, Location location, core::StringView format, Args&&... args)
	{
		auto message = core::strf(allocator, format, std::forward<Args>(args)...);
		return Error{location, std::move(message)};
	}
}
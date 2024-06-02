#pragma once

#include "minijava/Token.h"
#include "minijava/Error.h"

#include <core/String.h>
#include <core/Result.h>
#include <core/Unique.h>
#include <core/Array.h>

namespace minijava
{
	class Unit
	{
	public:
		enum STAGE
		{
			STAGE_NONE,
			STAGE_FAILED,
			STAGE_SCANNED
		};

		static core::Result<core::Unique<Unit>> create(core::StringView path, core::Allocator* allocator);

		core::StringView content() const { return m_content; }
		void pushLine(core::StringView line) { m_lines.push(line); }
		void pushError(Error error) { m_errors.push(std::move(error)); }

	private:
		template<typename T, typename... TArgs>
		friend inline core::Unique<T> core::unique_from(core::Allocator* allocator, TArgs&&... args);

		core::String m_filePath;
		core::String m_absolutePath;
		core::String m_content;
		STAGE m_stage = STAGE_NONE;
		core::Array<Token> m_token;
		core::Array<Error> m_errors;
		core::Array<core::StringView> m_lines;

		Unit(core::String filePath, core::String absolutePath, core::String content, core::Allocator* allocator)
			: m_filePath(std::move(filePath)),
			  m_absolutePath(std::move(absolutePath)),
			  m_content(std::move(content)),
			  m_token(allocator),
			  m_errors(allocator),
			  m_lines(allocator)
		{}
	};
}
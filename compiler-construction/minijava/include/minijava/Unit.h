#pragma once

#include <core/String.h>
#include <core/Result.h>
#include <core/Unique.h>

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

	private:
		template<typename T, typename... TArgs>
		friend inline core::Unique<T> core::unique_from(core::Allocator* allocator, TArgs&&... args);

		core::String m_filePath;
		core::String m_absolutePath;
		core::String m_content;
		STAGE m_stage = STAGE_NONE;

		Unit(core::String filePath, core::String absolutePath, core::String content)
			: m_filePath(std::move(filePath)),
			  m_absolutePath(std::move(absolutePath)),
			  m_content(std::move(content))
		{}
	};
}
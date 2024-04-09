#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/String.h"

namespace core
{
	class Path
	{
		static size_t volumeNameLen(StringView path);

		static void join(String& result) {}

		template<typename ... TArgs>
		static void join(String& result, StringView first, TArgs&& ... args)
		{
			result.push(first);
			join(result, std::forward<TArgs>(args)...);
		}

	public:
		CORE_EXPORT static Result<String> abs(StringView path, Allocator* allocator);
		CORE_EXPORT static Result<String> workingDir(Allocator* allocator);

		template<typename ... TArgs>
		static String join(Allocator* allocator, TArgs&& ... args)
		{
			String result{allocator};
			join(result, std::forward<TArgs>(args)...);
			return result;
		}

		static String clean(StringView path, Allocator* allocator)
		{
			String result{allocator};

			path = path.trimLeft("\\/"_sv);
			Rune prev{'\0'};
			for (auto c: path.runes())
			{
				if (c == '\\' && prev != '\\')
				{
					result.push(Rune{'/'});
				}
				else if (c == '\\' && prev == '\\')
				{
					prev = c;
					continue;
				}
				else if (c == '/' && prev == '/')
				{
					prev = c;
					continue;
				}
				else
				{
					result.push(c);
				}
				prev = c;
			}
			if ((prev == '\\' || prev == '/') && result.count() > 0)
				result.resize(result.count() - 1);
			return result;
		}
	};
}
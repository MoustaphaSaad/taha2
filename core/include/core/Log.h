#pragma once

#include "core/StringView.h"
#include "core/Shared.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <utility>

namespace core
{
	class Log
	{
		Shared<spdlog::logger> m_logger;
	public:
		using level = spdlog::level::level_enum;

		explicit Log(Allocator* allocator)
		{
			auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			m_logger = shared_from<spdlog::logger>(allocator, "console", sink);
			setPattern("[%H:%M:%S %z] [thread %t] [%^%l%$] [%n] %v"_sv);
			setFlushLevel(level::err);

			#if defined(DEBUG)
			setLevel(level::trace);
			#endif
		}

		explicit Log(Shared<spdlog::logger> logger) : m_logger(std::move(logger)) {}

		void setLevel(level level)
		{
			m_logger->set_level(level);
		}

		void setPattern(StringView pattern)
		{
			m_logger->set_pattern(std::string{pattern.data(), pattern.count()});
		}

		void setFlushLevel(level level)
		{
			m_logger->flush_on(level);
		}

		template<typename ... TArgs>
		void trace(StringView format, TArgs&& ... args)
		{
			m_logger->trace(fmt::string_view{format.data(), format.count()}, std::forward<TArgs>(args)...);
		}

		template<typename ... TArgs>
		void debug(StringView format, TArgs&& ... args)
		{
			m_logger->debug(fmt::string_view{format.data(), format.count()}, std::forward<TArgs>(args)...);
		}

		template<typename ... TArgs>
		void info(StringView format, TArgs&& ... args)
		{
			m_logger->info(fmt::string_view{format.data(), format.count()}, std::forward<TArgs>(args)...);
		}

		template<typename ... TArgs>
		void error(StringView format, TArgs&& ... args)
		{
			m_logger->error(fmt::string_view{format.data(), format.count()}, std::forward<TArgs>(args)...);
		}

		template<typename ... TArgs>
		void warn(StringView format, TArgs&& ... args)
		{
			m_logger->warn(fmt::string_view{format.data(), format.count()}, std::forward<TArgs>(args)...);
		}

		template<typename ... TArgs>
		void critical(StringView format, TArgs&& ... args)
		{
			m_logger->critical(fmt::string_view{format.data(), format.count()}, std::forward<TArgs>(args)...);
		}
	};
}
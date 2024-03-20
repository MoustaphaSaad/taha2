#pragma once

#include "core/Stream.h"
#include "core/MemoryStream.h"
#include <cpptrace/cpptrace.hpp>

namespace core
{
	namespace detail
	{
		class StreamBuffer: public std::streambuf
		{
			Stream* m_stream = nullptr;
		public:
			StreamBuffer(Stream* stream)
				: m_stream(stream)
			{}

		protected:
			int_type overflow(int_type c) override
			{
				if (c != EOF)
				{
					auto charC = (char)c;
					m_stream->write(&charC, sizeof(charC));
				}
				return c;
			}

			std::streamsize xsputn(const char* str, std::streamsize n) override
			{
				return m_stream->write(str, n);
			}
		};

		class OutputStream: public std::ostream
		{
			StreamBuffer m_streamBuffer;
		public:
			OutputStream(Stream* stream)
				: std::ostream(nullptr),
				  m_streamBuffer(stream)
			{
				rdbuf(&m_streamBuffer);
			}
		};
	}

	class Stacktrace
	{
		cpptrace::stacktrace m_trace;
	public:
		explicit Stacktrace(cpptrace::stacktrace trace)
			: m_trace(std::move(trace))
		{}

		static Stacktrace current(size_t skip, size_t maxDepth)
		{
			return Stacktrace{cpptrace::generate_trace(skip + 1, maxDepth)};
		}

		void print(Stream* stream, bool color) const
		{
			detail::OutputStream output{stream};
			m_trace.print(output, color);
		}

		String toString(Allocator* allocator, bool color = false) const
		{
			MemoryStream stream{allocator};
			print(&stream, color);
			return stream.releaseString();
		}
	};

	class Rawtrace
	{
		cpptrace::raw_trace m_trace;
	public:
		explicit Rawtrace(cpptrace::raw_trace trace)
			: m_trace(std::move(trace))
		{}

		static Rawtrace current(size_t skip, size_t maxDepth)
		{
			return Rawtrace{cpptrace::generate_raw_trace(skip + 1, maxDepth)};
		}

		Stacktrace resolve()
		{
			return Stacktrace{m_trace.resolve()};
		}
	};
}
#include "core/websocket/Handshake.h"

namespace core::websocket
{
	Result<Handshake> Handshake::parse(StringView request, Allocator* allocator)
	{
		size_t findIndex = 0;
		auto lineEnd = request.find(Rune{'\r'}, findIndex);
		if (lineEnd == SIZE_MAX) return errf(allocator, "cannot find request line end"_sv);
		auto requestLine = request.slice(findIndex, lineEnd);

		if (requestLine.endsWithIgnoreCase("http/1.1"_sv) == false)
			return errf(allocator, "unsupported http version"_sv);

		int requiredHeaders = 0;
		StringView key;

		findIndex = lineEnd + 2;
		while (request.count() - findIndex > 4)
		{
			lineEnd = request.find(Rune{'\r'}, findIndex);
			if (lineEnd == SIZE_MAX) return errf(allocator, "cannot find header line end"_sv);
			auto headerLine = request.slice(findIndex, lineEnd);

			auto separatorIndex = headerLine.find(Rune{':'});
			if (separatorIndex == SIZE_MAX) return errf(allocator, "cannot find header separator"_sv);

			auto headerName = headerLine.slice(0, separatorIndex).trim();
			auto headerValue = headerLine.slice(separatorIndex + 1, headerLine.count()).trim();

			if (headerName.equalsIgnoreCase("upgrade"_sv))
			{
				if (headerValue.equalsIgnoreCase("websocket"_sv) == false)
					return errf(allocator, "unsupported upgrade value"_sv);
				++requiredHeaders;
			}
			else if (headerName.equalsIgnoreCase("sec-websocket-version"_sv))
			{
				if (headerValue.equalsIgnoreCase("13"_sv) == false)
					return errf(allocator, "unsupported websocket version"_sv);
				++requiredHeaders;
			}
			else if (headerName.equalsIgnoreCase("connection"_sv))
			{
				if (headerValue.findIgnoreCase("upgrade"_sv) == SIZE_MAX)
					return errf(allocator, "unsupported connection value"_sv);
				++requiredHeaders;
			}
			else if (headerName.equalsIgnoreCase("sec-websocket-key"_sv))
			{
				key = headerValue;
				++requiredHeaders;
			}

			findIndex = lineEnd + 2;
		}

		if (requiredHeaders != 4)
			return errf(allocator, "missing required headers"_sv);

		return Handshake{String{key, allocator}};
	}
}
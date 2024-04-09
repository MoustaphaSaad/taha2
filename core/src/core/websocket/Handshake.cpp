#include "core/websocket/Handshake.h"

#include <tracy/Tracy.hpp>

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

			auto headerName = headerLine.sliceLeft(separatorIndex).trim();
			auto headerValue = headerLine.sliceRight(separatorIndex + 1).trim();

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

	Result<Handshake> Handshake::parseResponse(StringView response, Allocator *allocator)
	{
		ZoneScoped;

		auto lines = response.split("\r\n"_sv, true, allocator);
		if (lines.count() == 0)
			return errf(allocator, "failed to parse empty handshake response"_sv);

		auto statusLine = lines[0];
		if (statusLine.startsWithIgnoreCase("HTTP/1.1 101"_sv) == false)
			return errf(allocator, "unexpected handshake HTTP upgrade response, {}"_sv, statusLine);

		int validResponse = 0;
		StringView responseKey{};
		for (size_t i = 1; i < lines.count(); ++i)
		{
			auto line = lines[i];

			auto colonIndex = line.find(Rune{':'}, 0);
			if (colonIndex == SIZE_MAX)
				continue;

			auto key = line.sliceLeft(colonIndex).trim();
			auto value = line.sliceRight(colonIndex + 1).trim();

			if (key.equalsIgnoreCase("upgrade"_sv))
			{
				if (value.equalsIgnoreCase("websocket"_sv) == false)
					return errf(allocator, "invalid upgrade header, {}"_sv, value);
				++validResponse;
			}
			else if (key.equalsIgnoreCase("connection"_sv))
			{
				if (value.equalsIgnoreCase("upgrade"_sv) == false)
					return errf(allocator, "invalid connection header, {}"_sv, value);
				++validResponse;
			}
			else if (key.equalsIgnoreCase("sec-websocket-accept"_sv))
			{
				responseKey = value;
				++validResponse;
			}
		}

		if (validResponse != 3)
			return errf(allocator, "missing headers in handshake response"_sv);

		return Handshake{String{responseKey, allocator}};
	}
}
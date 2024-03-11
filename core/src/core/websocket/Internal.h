#pragma once

#include "core/websocket/Client2.h"
#include "core/Url.h"
#include "core/Unique.h"

namespace core::websocket
{
	class ImplClient2: public Client2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
	public:
		ImplClient2(EventSocket2 socket, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_socket(socket)
		{}

		static Result<Unique<Client2>> connect(const Client2Config& config, EventLoop2* loop, Log* log, Allocator* allocator)
		{
			auto parsedUrlResult = core::Url::parse(config.url, allocator);
			if (parsedUrlResult.isError())
				return parsedUrlResult.releaseError();
			auto parsedUrl = parsedUrlResult.releaseValue();

			auto socket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
			if (socket == nullptr)
				return errf(allocator, "failed to open socket"_sv);

			auto connected = socket->connect(parsedUrl.host(), parsedUrl.port());
			if (connected == false)
				return errf(allocator, "failed to connect to {}"_sv, config.url);

			auto socketSourceResult = loop->registerSocket(std::move(socket));
			if (socketSourceResult.isError())
				return socketSourceResult.releaseError();
			auto socketSource = socketSourceResult.releaseValue();

			return unique_from<ImplClient2>(allocator, socketSource, log, allocator);
		}
	};
}
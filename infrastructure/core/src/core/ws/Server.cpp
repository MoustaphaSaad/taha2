#include "core/ws/Server.h"
#include "core/Url.h"

namespace core::ws
{
	Result<Server>
	Server::connect(StringView url, size_t maxHandshakeSize, size_t maxMessageSize, Log* log, Allocator* allocator)
	{
		auto parsedUrlResult = Url::parse(url, allocator);
		if (parsedUrlResult.isError())
		{
			return parsedUrlResult.releaseError();
		}
		auto parsedUrl = parsedUrlResult.releaseValue();

		auto socket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
		if (socket == nullptr)
		{
			return errf(allocator, "failed to open server socket"_sv);
		}

		auto ok = socket->bind(parsedUrl.host(), parsedUrl.port());
		if (ok == false)
		{
			return errf(allocator, "failed to bind socket to '{}'"_sv, url);
		}

		ok = socket->listen();
		if (ok == false)
		{
			return errf(allocator, "failed to listen to socket"_sv);
		}

		return Server{std::move(socket), maxHandshakeSize, maxMessageSize, log, allocator};
	}

	Result<Client> Server::accept()
	{
		while (m_socket->listen())
		{
			auto clientSocket = m_socket->accept();
			if (clientSocket == nullptr)
			{
				return errf(m_allocator, "failed to accept client socket"_sv);
			}

			auto clientResult = Client::acceptFromServer(
				std::move(clientSocket), m_maxHandshakeSize, m_maxMessageSize, m_log, m_allocator);
			if (clientResult.isError())
			{
				continue;
			}
			return clientResult.releaseValue();
		}
		return errf(m_allocator, "failed ot listen to server socket"_sv);
	}

	void Server::close()
	{
		m_socket->shutdown(Socket::SHUTDOWN_RDWR);
		m_socket->close();
	}
}
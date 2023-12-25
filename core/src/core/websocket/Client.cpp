#include "core/websocket/Client.h"
#include "core/Rand.h"
#include "core/MemoryStream.h"
#include "core/Base64.h"

namespace core::websocket
{
	HumanError Client::handshake(core::StringView path)
	{
		MemoryStream request{m_allocator};

		auto base64Key = Base64::encode(Span<std::byte>{m_key, sizeof(m_key)}, m_allocator);

		// build the request text
		strf(&request, "GET {} HTTP/1.1\r\n"_sv, path);
		strf(&request, "content-length: 0\r\n"_sv);
		strf(&request, "upgrade: websocket\r\n"_sv);
		strf(&request, "sec-websocket-version: 13\r\n"_sv);
		strf(&request, "connection: upgrade\r\n"_sv);
		strf(&request, "sec-websocket-key: {}\r\n"_sv, base64Key);
		strf(&request, "\r\n"_sv);

		// send the built request
		auto requestAsString = request.releaseString();
		auto writtenSize = m_socket->write(requestAsString.data(), requestAsString.count());
		if (writtenSize != requestAsString.count())
			return errf(m_allocator, "failed to send handshake request"_sv);

		// read the response till the end
		String response{m_allocator};
		char line[1024] = {};
		while (true)
		{
			auto readBytes = m_socket->read(line, sizeof(line));
			if (readBytes == 0)
				return errf(m_allocator, "failed to find the end of response"_sv);
			else
				response.push(StringView{line, readBytes});

			if (response.endsWith("\r\n\r\n"_sv))
			{
				//TODO: now we have th entire response, parse it
				m_logger->debug("{}"_sv, response);
				break;
			}
		}
		return {};
	}

	Result<Unique<Client>> Client::connect(StringView ip, StringView port, Log *log, Allocator *allocator)
	{
		auto socket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
		if (socket == nullptr)
			return errf(allocator, "failed to open socket"_sv);

		auto connected = socket->connect(ip, port);
		if (connected == false)
			return errf(allocator, "failed to connect to {}:{}"_sv, ip, port);

		auto client = unique_from<Client>(allocator, std::move(socket), log, allocator);

		auto generatedKey = Rand::cryptoRand(Span<std::byte>{client->m_key, sizeof(client->m_key)});
		if (generatedKey == false)
			return errf(allocator, "failed to generate client key"_sv);

		if (auto err = client->handshake("/"_sv)) return err;

		return client;
	}
}
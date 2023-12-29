#include "core/websocket/Client.h"
#include "core/websocket/MessageParser.h"
#include "core/Rand.h"
#include "core/MemoryStream.h"
#include "core/Base64.h"
#include "core/SHA1.h"

#include <tracy/Tracy.hpp>

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
		bool receivedResponse = false;
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
				receivedResponse = true;
				break;
			}
		}

		auto lines = response.split("\r\n"_sv, true, m_allocator);
		if (lines.count() == 0)
			return errf(m_allocator, "failed to parse empty handshake response"_sv);

		auto statusLine = lines[0];
		if (statusLine.startsWithIgnoreCase("HTTP/1.1 101 "_sv) == false)
			return errf(m_allocator, "unexpected handshake http upgrade response, {}"_sv, statusLine);

		int validResponse = 0;
		for (size_t i = 1; i < lines.count(); ++i)
		{
			auto line = lines[i];

			auto colonIndex = line.find(Rune{':'}, 0);
			if (colonIndex == SIZE_MAX)
				continue;

			auto key = line.slice(0, colonIndex).trim();
			auto value = line.slice(colonIndex + 1, line.count()).trim();
			if (key.equalsIgnoreCase("upgrade"_sv))
			{
				if (value.equalsIgnoreCase("websocket"_sv) == false)
					return errf(m_allocator, "invalid upgrade header, {}"_sv, value);
				++validResponse;
			}
			else if (key.equalsIgnoreCase("connection"_sv))
			{
				if (value.equalsIgnoreCase("upgrade"_sv) == false)
					return errf(m_allocator, "invalid connection header, {}"_sv, value);
				++validResponse;
			}
			else if (key.equalsIgnoreCase("sec-websocket-accept"_sv))
			{
				core::SHA1Hasher hasher;
				hasher.hash(Base64::encode(Span<std::byte>{m_key, sizeof(m_key)}, m_allocator));
				hasher.hash("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv);
				auto secHash = hasher.final();
				auto base64SecHash = Base64::encode(secHash.asBytes(), m_allocator);
				if (base64SecHash != value)
					return errf(m_allocator, "invalid websocket accept header"_sv);
				++validResponse;
			}
		}

		if (validResponse != 3)
			return errf(m_allocator, "missing headers in handshake response"_sv);

		m_logger->debug("parsed handshake response successfully"_sv);
		return {};
	}

	HumanError Client::readAndHandleMessage()
	{
		// consume the bytes and encoded messages within it
		auto recvBytesCount = m_socket->read(m_recvLine, sizeof(m_recvLine));
		m_recvBuffer.push(Span<const std::byte>{m_recvLine, recvBytesCount});
		auto recvBytes = Span<const std::byte>{m_recvBuffer};
		while (recvBytes.count() > 0)
		{
			auto parserResult = m_messageParser.consume(recvBytes);
			if (parserResult.isError())
			{
				// TODO: close the connection here
				return parserResult.releaseError();
			}
			auto consumedBytes = parserResult.value();

			// if we didn't consume any bytes we just wait for more bytes
			if (consumedBytes == 0)
				break;

			recvBytes = recvBytes.slice(consumedBytes, recvBytes.count() - consumedBytes);

			if (m_messageParser.hasMessage())
			{
				auto msg = m_messageParser.message();
				m_logger->debug("type: {}, payload: {}"_sv, (int)msg.type, StringView{msg.payload});
				if (auto err = onMsg(msg)) return err;
			}
		}

		if (recvBytes.count() == 0)
		{
			m_recvBuffer.clear();
		}
		else
		{
			::memcpy(m_recvBuffer.data(), recvBytes.data(), recvBytes.count());
			m_recvBuffer.resize(recvBytes.count());
		}
		return {};
	}

	HumanError Client::onMsg(const Message& msg)
	{
		switch (msg.type)
		{
		case Message::TYPE_TEXT:
		{
			auto text = StringView{msg.payload};
			if (text.isValidUtf8() == false)
				return errf(m_allocator, "invalid utf8 string"_sv);

			if (m_config.onMsg)
			{
				ZoneScoped;
				return m_config.onMsg(msg, this);
			}
			return {};
		}
		case Message::TYPE_BINARY:
		{
			if (m_config.onMsg)
			{
				ZoneScoped;
				return m_config.onMsg(msg, this);
			}
			return {};
		}
		case Message::TYPE_CLOSE:
		{
			if (m_config.handleClose)
			{
				ZoneScoped;
				return m_config.onMsg(msg, this);
			}
			else
			{
				// TODO: handle message
				return errf(m_allocator, "unimplemented"_sv);
			}
		}
		case Message::TYPE_PING:
		{
			if (m_config.handlePing)
			{
				ZoneScoped;
				return m_config.onMsg(msg, this);
			}
			else
			{
				// TODO: handle message
				return errf(m_allocator, "unimplemented"_sv);
			}
		}
		case Message::TYPE_PONG:
		{
			if (m_config.handlePong)
			{
				ZoneScoped;
				return m_config.onMsg(msg, this);
			}
			else
			{
				// TODO: handle message
				return errf(m_allocator, "unimplemented"_sv);
			}
		}
		default:
		{
			assert(false);
			return errf(m_allocator, "Invalid message type"_sv);
		}
		}
	}

	Result<Unique<Client>> Client::connect(ClientConfig&& config, Log *log, Allocator *allocator)
	{
		auto socket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
		if (socket == nullptr)
			return errf(allocator, "failed to open socket"_sv);

		auto connected = socket->connect(config.ip, config.port);
		if (connected == false)
			return errf(allocator, "failed to connect to {}:{}"_sv, config.ip, config.port);

		auto client = unique_from<Client>(allocator, std::move(config), std::move(socket), log, allocator);

		auto generatedKey = Rand::cryptoRand(Span<std::byte>{client->m_key, sizeof(client->m_key)});
		if (generatedKey == false)
			return errf(allocator, "failed to generate client key"_sv);

		if (auto err = client->handshake("/"_sv)) return err;

		return client;
	}

	HumanError Client::run()
	{
		bool running = true;
		while (running)
		{
			if (auto err = readAndHandleMessage())
			{
				// TODO: close the connection here
				return err;
			}
		}

		return {};
	}
}
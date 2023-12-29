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
				if (msg.payload.count() == 0)
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});

				// error protocol close payload should have at least 2 byte
				if (msg.payload.count() == 1)
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)});

				auto errorCode = uint16_t(msg.payload[1]) | (uint16_t(msg.payload[0]) << 8);
				if (errorCode < 1000 || errorCode == 1004 || errorCode == 1005 || errorCode == 1006 || (errorCode > 1013  && errorCode < 3000))
				{
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)});
				}

				if (msg.payload.count() == 2)
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});

				// close payload should be utf8
				auto payload = StringView{msg.payload}.slice(2, msg.payload.count());
				if (payload.isValidUtf8() == false)
				{
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)});
				}

				return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});
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
				if (msg.payload.count() == 0)
				{
					return writeRaw(Span<const std::byte>{(const std::byte *) EMPTY_PONG, sizeof(EMPTY_PONG)});
				}
				else
				{
					return writePong(Span<const std::byte>{msg.payload});
				}
			}
		}
		case Message::TYPE_PONG:
		{
			if (m_config.handlePong)
			{
				ZoneScoped;
				return m_config.onMsg(msg, this);
			}
			return {};
		}
		default:
		{
			assert(false);
			return errf(m_allocator, "Invalid message type"_sv);
		}
		}
	}

	HumanError Client::writeRaw(Span<const std::byte> bytes)
	{
		auto remainingBytes = bytes;
		while (remainingBytes.count() > 0)
		{
			auto writtenCount = m_socket->write(remainingBytes.data(), remainingBytes.count());
			remainingBytes = remainingBytes.slice(writtenCount, remainingBytes.count() - writtenCount);
		}
		return {};
	}

	HumanError Client::writeFrameHeader(FrameHeader::OPCODE opcode, size_t payloadLength)
	{
		std::byte buf[10];
		buf[0] = std::byte(128 | opcode);

		if (payloadLength <= 125)
		{
			buf[1] = std::byte(payloadLength);
			if (auto err = writeRaw(Span<const std::byte>{buf, 2})) return err;
		}
		else if (payloadLength <= UINT16_MAX)
		{
			buf[1] = std::byte(126);
			buf[2] = std::byte((payloadLength >> 8) & 0xFF);
			buf[3] = std::byte(payloadLength & 0xFF);
			if (auto err = writeRaw(Span<const std::byte>{buf, 4})) return err;
		}
		else
		{
			buf[1] = std::byte(127);
			buf[2] = std::byte((payloadLength >> 56) & 0xFF);
			buf[3] = std::byte((payloadLength >> 48) & 0xFF);
			buf[4] = std::byte((payloadLength >> 40) & 0xFF);
			buf[5] = std::byte((payloadLength >> 32) & 0xFF);
			buf[6] = std::byte((payloadLength >> 24) & 0xFF);
			buf[7] = std::byte((payloadLength >> 16) & 0xFF);
			buf[8] = std::byte((payloadLength >> 8) & 0xFF);
			buf[9] = std::byte(payloadLength & 0xFF);
			if (auto err = writeRaw(Span<const std::byte>{buf, 10})) return err;
		}

		return {};
	}

	HumanError Client::writeFrame(FrameHeader::OPCODE opcode, Span<const std::byte> payload)
	{
		if (auto err = writeFrameHeader(opcode, payload.count())) return err;
		if (auto err = writeRaw(payload)) return err;
		return {};
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
				m_logger->debug("Failed to read and handle message: {}"_sv, err);
				(void) writeRaw(Span<const std::byte>{(const std::byte*) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)});
				return err;
			}
		}

		return {};
	}

	HumanError Client::writeText(StringView str)
	{
		return writeFrame(FrameHeader::OPCODE_TEXT, Span<const std::byte>{str});
	}

	HumanError Client::writeBinary(Span<const std::byte> bytes)
	{
		return writeFrame(FrameHeader::OPCODE_BINARY, bytes);
	}

	HumanError Client::writePing(Span<const std::byte> bytes)
	{
		return writeFrame(FrameHeader::OPCODE_PING, bytes);
	}

	HumanError Client::writePong(Span<const std::byte> bytes)
	{
		return writeFrame(FrameHeader::OPCODE_PONG, bytes);
	}

	HumanError Client::writeClose()
	{
		return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});
	}

	HumanError Client::writeClose(uint16_t code)
	{
		uint8_t buf[2];
		buf[0] = (code >> 8) & 0xFF;
		buf[1] = code & 0xFF;
		return writeFrame(FrameHeader::OPCODE_CLOSE, Span<const std::byte>{(const std::byte*)buf, 2});
	}
}
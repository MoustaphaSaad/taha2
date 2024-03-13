#include "core/websocket/Client3.h"
#include "core/websocket/Server3.h"
#include "core/websocket/Handshake.h"
#include "core/Buffer.h"
#include "core/SHA1.h"
#include "core/Base64.h"
#include "core/Rand.h"

#include <tracy/Tracy.hpp>

namespace core::websocket
{
	class ServerHandshakeThread3: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		Buffer m_buffer;
		size_t m_maxHandshakeSize = 1ULL * 1024ULL;
		Client3* m_client = nullptr;
		bool m_success = false;
	public:
		ServerHandshakeThread3(EventLoop2* loop, Client3* client, EventSocket2 socket, size_t maxHandshakeSize, Log* log, Allocator* allocator)
			: EventThread2(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_buffer(allocator),
			  m_maxHandshakeSize(maxHandshakeSize),
			  m_client(client)
		{}

		~ServerHandshakeThread3() override
		{
			m_client->handshakeDone(m_success);
		}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				ZoneScopedN("StartEvent");
				return m_socket.read(sharedFromThis());
			}
			else if (auto readEvent = dynamic_cast<ReadEvent2*>(event))
			{
				ZoneScopedN("ReadEvent");
				auto totalHandshakeBuffer = m_buffer.count() + readEvent->bytes().count();
				if (totalHandshakeBuffer > m_maxHandshakeSize)
				{
					// TODO: maybe wait until data is sent
					m_log->error("handshake size is too long, size: {}bytes, max: {}bytes"_sv, totalHandshakeBuffer, m_maxHandshakeSize);
					(void)m_socket.write(R"(HTTP/1.1 400 Invalid\r\nerror: too large\r\ncontent-length: 0\r\n\r\n)"_sv, nullptr);
					return stop();
				}
				m_buffer.push(readEvent->bytes());

				auto handshakeString = StringView{m_buffer};
				if (handshakeString.endsWith("\r\n\r\n"_sv))
				{
					auto handshakeResult = Handshake::parse(handshakeString, m_allocator);
					if (handshakeResult.isError())
					{
						// TODO: maybe wait until data is sent
						(void)m_socket.write(R"(HTTP/1.1 400 Invalid\r\nerror: failed to parse handshake\r\ncontent-length: 0\r\n\r\n)"_sv, nullptr);
						return stop();
					}
					auto handshake = handshakeResult.releaseValue();

					constexpr static const char* REPLY = "HTTP/1.1 101 Switching Protocols\r\n"
						"Upgrade: websocket\r\n"
						"Connection: Upgrade\r\n"
						"Sec-WebSocket-Accept: {}\r\n"
						"\r\n";
					auto concatKey = strf(m_allocator, "{}{}"_sv, handshake.key(), "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv);
					auto sha1 = SHA1::hash(concatKey);
					auto base64 = Base64::encode(sha1.asBytes(), m_allocator);
					auto reply = strf(m_allocator, StringView{REPLY, strlen(REPLY)}, base64);
					if (auto err = m_socket.write(Span<const std::byte>{reply}, nullptr))
					{
						m_log->error("failed to send handshake reply, {}"_sv, err);
						return stop();
					}

					m_success = true;
					return stop();
				}
				else
				{
					return m_socket.read(sharedFromThis());
				}
			}
			return {};
		}
	};

	class ReadMessageThread3: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		Buffer m_buffer;
		MessageParser m_messageParser;
		Shared<EventThread2> m_handler;
		Client3* m_client = nullptr;

	public:
		ReadMessageThread3(EventLoop2* loop, Client3* client, const Shared<EventThread2>& handler, EventSocket2 socket, size_t maxMessageSize, Log* log, Allocator* allocator)
			: EventThread2(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_buffer(allocator),
			  m_messageParser(allocator, maxMessageSize),
			  m_handler(handler),
			  m_client(client)
		{}

		~ReadMessageThread3() override
		{
			m_client->connectionClosed();
		}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				ZoneScopedN("StartEvent");
				return m_socket.read(sharedFromThis());
			}
			else if (auto readEvent = dynamic_cast<ReadEvent2*>(event))
			{
				ZoneScopedN("ReadEvent");
				m_buffer.push(readEvent->bytes());

				auto bytes = Span<const std::byte>{m_buffer};
				while (bytes.count() > 0)
				{
					auto parserResult = m_messageParser.consume(bytes);
					if (parserResult.isError())
					{
						auto errorEvent = unique_from<ErrorEvent>(m_allocator, m_client->sharedFromThis(), 1002, parserResult.releaseError());
						(void) m_handler->send(std::move(errorEvent));
						return stop();
					}
					auto consumedBytes = parserResult.value();

					// if we didn't consume any bytes we just wait for more bytes
					if (consumedBytes == 0 && m_messageParser.hasMessage() == false)
						break;

					if (consumedBytes > 0)
						bytes = bytes.slice(consumedBytes, bytes.count() - consumedBytes);

					if (m_messageParser.hasMessage())
					{
						auto msg = m_messageParser.message();

						if (msg.type == Message::TYPE_TEXT && StringView{msg.payload}.isValidUtf8() == false)
						{
							auto errorEvent = unique_from<ErrorEvent>(m_allocator, m_client->sharedFromThis(), 1007, errf(m_allocator, "invalid utf8"_sv));
							(void) m_handler->send(std::move(errorEvent));
							return stop();
						}

						bool isClose = msg.type == Message::TYPE_CLOSE;
						auto messageEvent = unique_from<MessageEvent>(m_allocator, m_client->sharedFromThis(), std::move(msg));
						if (auto err = m_handler->send(std::move(messageEvent)))
							return err;

						if (isClose)
							return stop();
					}
				}

				if (bytes.count() == 0)
				{
					m_buffer.clear();
				}
				else
				{
					::memmove(m_buffer.data(), bytes.data(), bytes.count());
					m_buffer.resize(bytes.count());
				}

				if (auto err = m_socket.read(sharedFromThis()))
					return stop();
				return {};
			}
			return {};
		}
	};

	HumanError Client3::writeFrame(FrameHeader::OPCODE opcode, Span<const std::byte> payload)
	{
		ZoneScoped;
		if (FrameHeader::isCtrlOpcode(opcode))
		{
			assert(payload.sizeInBytes() <= 125);
			if (payload.sizeInBytes() > 125)
				payload = payload.slice(0, 125);
		}

		bool shouldMask = false;
		std::byte mask[4] = {};
		if (shouldMask)
		{
			if (Rand::cryptoRand(Span<std::byte>{mask, sizeof(mask)}) == false)
				return errf(m_allocator, "failed to generate random mask"_sv);
		}

		std::byte buf[14] = {};
		buf[0] = std::byte(128 | opcode);
		size_t bufSize = 1;

		if (payload.count() <= 125)
		{
			buf[1] = std::byte(payload.count());
			++bufSize;
		}
		else if (payload.count() <= UINT16_MAX)
		{
			buf[1] = std::byte(126);
			buf[2] = std::byte((payload.count() >> 8) & 0xFF);
			buf[3] = std::byte(payload.count() & 0xFF);
			bufSize += 3;
		}
		else
		{
			buf[1] = std::byte(127);
			buf[2] = std::byte((payload.count() >> 56) & 0xFF);
			buf[3] = std::byte((payload.count() >> 48) & 0xFF);
			buf[4] = std::byte((payload.count() >> 40) & 0xFF);
			buf[5] = std::byte((payload.count() >> 32) & 0xFF);
			buf[6] = std::byte((payload.count() >> 24) & 0xFF);
			buf[7] = std::byte((payload.count() >> 16) & 0xFF);
			buf[8] = std::byte((payload.count() >> 8) & 0xFF);
			buf[9] = std::byte(payload.count() & 0xFF);
			bufSize += 9;
		}

		if (shouldMask)
		{
			buf[1] |= std::byte(128);
			::memcpy(buf + bufSize, mask, sizeof(mask));
			bufSize += sizeof(mask);
		}

		if (auto err = m_socket.write(Span<const std::byte>{buf, bufSize}, nullptr))
			return err;

		if (shouldMask)
		{
			Buffer maskedPayload{m_allocator};
			maskedPayload.push(payload);
			for (size_t i = 0; i < maskedPayload.count(); ++i)
				maskedPayload[i] ^= mask[i & 3];
			return m_socket.write(Span<const std::byte>{maskedPayload}, nullptr);
		}
		else
		{
			return m_socket.write(payload, nullptr);
		}
	}

	HumanError Client3::writeCloseWithCode(uint16_t code, StringView reason)
	{
		ZoneScoped;
		std::byte buf[125];
		buf[0] = std::byte((code >> 8) & 0xFF);
		buf[1] = std::byte(code & 0xFF);
		auto maxReasonSize = reason.count() > 123 ? 0 : reason.count();
		::memcpy(buf + 2, reason.data(), maxReasonSize);
		auto payloadSize = 2 + maxReasonSize;
		return writeFrame(FrameHeader::OPCODE_CLOSE, {buf, payloadSize});
	}

	void Client3::handshakeDone(bool success)
	{
		ZoneScoped;
		if (success)
		{
			m_server->clientHandshakeDone(sharedFromThis());
		}
		else
		{
			// TODO: handle failure here
		}
	}

	void Client3::connectionClosed()
	{
		ZoneScoped;
		m_server->clientClosed(sharedFromThis());
	}

	Client3::Client3(EventSocket2 socket, size_t maxMessageSize, Log* log, Allocator* allocator)
		: m_allocator(allocator),
		  m_log(log),
		  m_socket(socket),
		  m_maxMessageSize(maxMessageSize)
	{}

	Shared<Client3> Client3::acceptFromServer(
		Server3* server,
		EventLoop2* loop,
		EventSocket2 socket,
		size_t maxHandshakeSize,
		size_t maxMessageSize,
		Log* log,
		Allocator* allocator)
	{
		ZoneScoped;
		auto res = shared_from<Client3>(allocator, socket, maxMessageSize, log, allocator);
		res->m_server = server;
		loop->startThread<ServerHandshakeThread3>(loop, res.get(), socket, maxHandshakeSize, log, allocator);
		return res;
	}

	HumanError Client3::startReadingMessages(const Shared<EventThread2>& handler)
	{
		ZoneScoped;
		auto loop = handler->eventLoop();
		loop->startThread<ReadMessageThread3>(loop, this, handler, m_socket, m_maxMessageSize, m_log, m_allocator);
		return {};
	}

	HumanError Client3::writeText(StringView payload)
	{
		ZoneScoped;
		return writeFrame(FrameHeader::OPCODE_TEXT, payload);
	}

	HumanError Client3::writeBinary(Span<const std::byte> payload)
	{
		ZoneScoped;
		return writeFrame(FrameHeader::OPCODE_BINARY, payload);
	}

	HumanError Client3::writePing(Span<const std::byte> payload)
	{
		ZoneScoped;
		return writeFrame(FrameHeader::OPCODE_PING, payload);
	}

	HumanError Client3::writePong(Span<const std::byte> payload)
	{
		ZoneScoped;
		return writeFrame(FrameHeader::OPCODE_PONG, payload);
	}

	HumanError Client3::writeClose(uint16_t errorCode, StringView optionalReason)
	{
		ZoneScoped;
		return writeCloseWithCode(errorCode, optionalReason);
	}

	HumanError Client3::defaultMessageHandler(const Message& message)
	{
		ZoneScoped;
		if (message.type == Message::TYPE_CLOSE)
		{
			m_log->debug("close: {}"_sv, message.payload.count());
			if (message.payload.count() == 0)
				return writeClose(1000);

			if (message.payload.count() == 1)
				return writeClose(1002);

			auto errorCode = uint16_t(message.payload[1]) | (uint16_t(message.payload[0]) << 8);
			if (errorCode < 1000 || errorCode == 1004 || errorCode == 1005 || errorCode == 1006 || (errorCode > 1013  && errorCode < 3000))
				return writeClose(1002);

			if (message.payload.count() == 2)
				return writeClose(1000);

			auto payload = StringView{message.payload}.slice(2, message.payload.count());
			if (payload.isValidUtf8() == false)
				return writeClose(1007);

			return writeClose(1000);
		}
		else if (message.type == Message::TYPE_PING)
		{
			m_log->debug("ping: {}"_sv, message.payload.count());
			return writePong(Span<const std::byte>{message.payload});
		}
		else if (message.type == Message::TYPE_PONG)
		{
			m_log->debug("pong: {}"_sv, message.payload.count());
		}
		return {};
	}
}
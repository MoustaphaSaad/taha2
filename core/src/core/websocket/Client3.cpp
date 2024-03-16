#include "core/websocket/Client3.h"
#include "core/websocket/Server3.h"
#include "core/websocket/Handshake.h"
#include "core/Buffer.h"
#include "core/SHA1.h"
#include "core/Base64.h"
#include "core/Rand.h"
#include "core/Url.h"
#include "core/MemoryStream.h"

#include <tracy/Tracy.hpp>

namespace core::websocket
{
	class ServerHandshakeThread3: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		size_t m_maxHandshakeSize = 1ULL * 1024ULL;
		Client3* m_client = nullptr;
		bool m_success = false;
	public:
		ServerHandshakeThread3(EventLoop2* loop, Client3* client, EventSocket2 socket, size_t maxHandshakeSize, Log* log, Allocator* allocator)
			: EventThread2(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
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
				auto& buffer = m_client->m_buffer;

				auto totalHandshakeBuffer = buffer.count() + readEvent->bytes().count();
				if (totalHandshakeBuffer > m_maxHandshakeSize)
				{
					// TODO: maybe wait until data is sent
					m_log->error("handshake size is too long, size: {}bytes, max: {}bytes"_sv, totalHandshakeBuffer, m_maxHandshakeSize);
					(void)m_socket.write(R"(HTTP/1.1 400 Invalid\r\nerror: too large\r\ncontent-length: 0\r\n\r\n)"_sv, nullptr);
					return stop();
				}
				buffer.push(readEvent->bytes());

				auto handshakeString = StringView{buffer};
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

					buffer.clear();
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

	class ClientHandshakeThread3: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		size_t m_maxHandshakeSize = 1ULL * 1024ULL;
		Client3* m_client = nullptr;
		std::byte m_key[16] = {};
		Url m_url;
		bool m_success = false;
	public:
		ClientHandshakeThread3(EventLoop2* loop, Client3* client, EventSocket2 socket, size_t maxHandshakeSize, Span<const std::byte> key, Url url, Log* log, Allocator* allocator)
			: EventThread2(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_maxHandshakeSize(maxHandshakeSize),
			  m_client(client),
			  m_url(std::move(url))
		{
			assert(key.sizeInBytes() == sizeof(m_key));
			::memcpy(m_key, key.data(), key.sizeInBytes());
		}

		~ClientHandshakeThread3() override
		{
			m_client->handshakeDone(m_success);
		}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				ZoneScopedN("StartEvent");
				MemoryStream request{m_allocator};

				auto base64Key = Base64::encode(Span<std::byte>{m_key, sizeof(m_key)}, m_allocator);
				auto pathResult = m_url.pathWithQueryAndFragment(m_allocator);
				if (pathResult.isError())
					return pathResult.releaseError();
				auto path = pathResult.releaseValue();

				strf(&request, "GET {} HTTP/1.1\r\n"_sv, path);
				strf(&request, "content-length: 0\r\n"_sv);
				strf(&request, "upgrade: websocket\r\n"_sv);
				strf(&request, "sec-websocket-version: 13\r\n"_sv);
				strf(&request, "connection: upgrade\r\n"_sv);
				strf(&request, "sec-websocket-key: {}\r\n"_sv, base64Key);
				strf(&request, "Host: {}:{}\r\n"_sv, m_url.host(), m_url.port());
				strf(&request, "\r\n"_sv);

				auto requestAsString = request.releaseString();
				if (auto err = m_socket.write(Span<const std::byte>{requestAsString}, nullptr))
					return err;
				return m_socket.read(sharedFromThis());
			}
			else if (auto readEvent = dynamic_cast<ReadEvent2*>(event))
			{
				ZoneScopedN("ReadEvent");
				auto& buffer = m_client->m_buffer;

				auto totalHandshakeBuffer = buffer.count() + readEvent->bytes().count();
				if (totalHandshakeBuffer > m_maxHandshakeSize)
				{
					m_log->error("handshake size is too long, size: {}bytes, max: {}bytes"_sv, totalHandshakeBuffer, m_maxHandshakeSize);
					return stop();
				}
				buffer.push(readEvent->bytes());

				auto handshakeString = StringView{buffer};
				if (auto responseEnd = handshakeString.find("\r\n\r\n"_sv); responseEnd != SIZE_MAX)
				{
					auto handshakeResult = Handshake::parseResponse(handshakeString, m_allocator);
					if (handshakeResult.isError())
					{
						m_log->error("failed to parse handshake response, {}"_sv, handshakeResult.releaseError());
						return stop();
					}
					auto handshake = handshakeResult.releaseValue();

					core::SHA1Hasher hasher;
					hasher.hash(Base64::encode(Span<std::byte>{m_key, sizeof(m_key)}, m_allocator));
					hasher.hash("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv);
					auto secHash = hasher.final();
					auto base64SecHash = Base64::encode(secHash.asBytes(), m_allocator);
					if (base64SecHash != handshake.key())
					{
						m_log->error("invalid hanshaked key response"_sv);
						return stop();
					}

					// +4 for the last \r\n\r\n
					auto responseSize = responseEnd + 4;
					::memmove(buffer.data(), buffer.data() + responseSize, buffer.count() - responseSize);
					buffer.resize(buffer.count() - responseSize);
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
		MessageParser m_messageParser;
		Shared<EventThread2> m_handler;
		Client3* m_client = nullptr;

		HumanError handleReadBuffer(Buffer& buffer)
		{
			auto bytes = Span<const std::byte>{buffer};
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
				buffer.clear();
			}
			else
			{
				::memmove(buffer.data(), bytes.data(), bytes.count());
				buffer.resize(bytes.count());
			}
			return {};
		}

	public:
		ReadMessageThread3(EventLoop2* loop, Client3* client, const Shared<EventThread2>& handler, EventSocket2 socket, size_t maxMessageSize, Log* log, Allocator* allocator)
			: EventThread2(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
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
				auto& buffer = m_client->m_buffer;
				if (buffer.count() > 0)
				{
					if (auto err = handleReadBuffer(buffer))
						return err;
				}

				if (stopped() == false)
					return m_socket.read(sharedFromThis());

				return {};
			}
			else if (auto readEvent = dynamic_cast<ReadEvent2*>(event))
			{
				ZoneScopedN("ReadEvent");
				auto& buffer = m_client->m_buffer;
				buffer.push(readEvent->bytes());

				if (auto err = handleReadBuffer(buffer))
					return err;

				if (stopped() == false)
				{
					if (auto err = m_socket.read(sharedFromThis()))
						return stop();
				}

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

		bool shouldMask = m_server == nullptr;
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
			if (m_server)
			{
				m_server->clientHandshakeDone(sharedFromThis());
			}
			else
			{
				auto newConn = unique_from<NewConnection3>(m_allocator, sharedFromThis());
				(void)m_handler->send(std::move(newConn));
			}
		}
		else
		{
			// TODO: handle failure here
		}
	}

	void Client3::connectionClosed()
	{
		ZoneScoped;
		auto disconnected = unique_from<DisconnectedEvent>(m_allocator, sharedFromThis());
		(void)m_handler->send(std::move(disconnected));

		if (m_server)
			m_server->clientClosed(sharedFromThis());
	}

	Client3::Client3(EventSocket2 socket, size_t maxMessageSize, Log* log, Allocator* allocator)
		: m_allocator(allocator),
		  m_log(log),
		  m_socket(socket),
		  m_maxMessageSize(maxMessageSize),
		  m_buffer(allocator)
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

	Result<Shared<Client3>> Client3::connect(StringView url, size_t maxHandshakeSize, size_t maxMessageSize, const Shared<EventThread2>& handler, Log* log, Allocator* allocator)
	{
		ZoneScoped;
		auto parsedUrlResult = core::Url::parse(url, allocator);
		if (parsedUrlResult.isError())
			return parsedUrlResult.releaseError();
		auto parsedUrl = parsedUrlResult.releaseValue();

		std::byte key[16] = {};
		if (Rand::cryptoRand(Span<std::byte>{key, sizeof(key)}) == false)
			return errf(allocator, "failed to generate random key"_sv);

		auto socket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
		if (socket == nullptr)
			return errf(allocator, "failed to open socket"_sv);

		auto connected = socket->connect(parsedUrl.host(), parsedUrl.port());
		if (connected == false)
			return errf(allocator, "failed to connect to {}"_sv, url);

		auto loop = handler->eventLoop();
		auto socketSourceResult = loop->registerSocket(std::move(socket));
		if (socketSourceResult.isError())
			return socketSourceResult.releaseError();
		auto socketSource = socketSourceResult.releaseValue();

		auto res = shared_from<Client3>(allocator, socketSource, maxMessageSize, log, allocator);
		res->m_handler = handler;
		loop->startThread<ClientHandshakeThread3>(loop, res.get(), socketSource, maxHandshakeSize, Span<const std::byte>{key, sizeof(key)}, std::move(parsedUrl), log, allocator);
		return res;
	}

	HumanError Client3::startReadingMessages(const Shared<EventThread2>& handler)
	{
		ZoneScoped;
		auto loop = handler->eventLoop();
		m_handler = handler;
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
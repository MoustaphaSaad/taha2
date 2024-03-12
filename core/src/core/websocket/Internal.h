#pragma once

#include "core/websocket/Client2.h"
#include "core/websocket/Server2.h"
#include "core/websocket/Handshake.h"
#include "core/websocket/MessageParser.h"
#include "core/SHA1.h"
#include "core/Base64.h"
#include "core/Url.h"
#include "core/Unique.h"
#include "core/Rand.h"

namespace core::websocket
{
	class ReadMessageThread: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		Buffer m_buffer;
		MessageParser m_messageParser;

		HumanError writeFrame(FrameHeader::OPCODE opcode, Span<const std::byte> payload)
		{
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

		HumanError writeCloseWithCode(uint16_t code, StringView reason)
		{
			std::byte buf[125];
			buf[0] = std::byte((code >> 8) & 0xFF);
			buf[1] = std::byte(code & 0xFF);
			auto maxReasonSize = reason.count() > 123 ? 0 : reason.count();
			::memcpy(buf + 2, reason.data(), maxReasonSize);
			auto payloadSize = 2 + maxReasonSize;
			return writeFrame(FrameHeader::OPCODE_CLOSE, {buf, payloadSize});
		}
	public:
		ReadMessageThread(EventSocket2 socket, size_t maxMessageSize, EventLoop2* loop, Log* log, Allocator* allocator)
			: EventThread2(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_buffer(allocator),
			  m_messageParser(allocator, maxMessageSize)
		{}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				return m_socket.read(sharedFromThis());
			}
			else if (auto readEvent = dynamic_cast<ReadEvent2*>(event))
			{
				m_buffer.push(readEvent->bytes());

				auto bytes = Span<const std::byte>{m_buffer};
				while (bytes.count() > 0)
				{
					auto parserResult = m_messageParser.consume(bytes);
					if (parserResult.isError())
					{
						(void)writeCloseWithCode(1002, parserResult.error().message());
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
						if (msg.type == Message::TYPE_TEXT)
						{
							m_log->debug("text: {}"_sv, msg.payload.count());
							auto text = StringView{msg.payload};
							if (text.isValidUtf8() == false)
							{
								(void)writeCloseWithCode(1007, "invalid utf8"_sv);
								return stop();
							}
							if (auto err = writeFrame(FrameHeader::OPCODE_TEXT, Span<const std::byte>{msg.payload}))
								return err;
						}
						else if (msg.type == Message::TYPE_BINARY)
						{
							m_log->debug("binary: {}"_sv, msg.payload.count());
							if (auto err = writeFrame(FrameHeader::OPCODE_BINARY, Span<const std::byte>{msg.payload}))
								return err;
						}
						else if (msg.type == Message::TYPE_CLOSE)
						{
							m_log->debug("close: {}"_sv, msg.payload.count());
							if (msg.payload.count() == 0)
							{
								(void)writeCloseWithCode(1000, {});
								return stop();
							}

							if (msg.payload.count() == 1)
							{
								(void)writeCloseWithCode(1002, {});
								return stop();
							}

							auto errorCode = uint16_t(msg.payload[1]) | (uint16_t(msg.payload[0]) << 8);
							if (errorCode < 1000 || errorCode == 1004 || errorCode == 1005 || errorCode == 1006 || (errorCode > 1013  && errorCode < 3000))
							{
								(void)writeCloseWithCode(1002, {});
								return stop();
							}

							if (msg.payload.count() == 2)
							{
								(void)writeCloseWithCode(1000, {});
								return stop();
							}

							auto payload = StringView{msg.payload}.slice(2, msg.payload.count());
							if (payload.isValidUtf8() == false)
							{
								(void)writeCloseWithCode(1007, {});
								return stop();
							}

							(void)writeCloseWithCode(1000, {});
							return stop();
						}
						else if (msg.type == Message::TYPE_PING)
						{
							m_log->debug("ping: {}"_sv, msg.payload.count());
							if (auto err = writeFrame(FrameHeader::OPCODE_PONG, Span<const std::byte>{msg.payload}))
								return err;
						}
						else if (msg.type == Message::TYPE_PONG)
						{
							m_log->debug("pong: {}"_sv, msg.payload.count());
						}
						else
						{
							assert(false);
						}
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

	class ServerHandshakeThread: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		Shared<EventThread2> m_handler;
		Buffer m_buffer;
		size_t m_maxHandshakeSize = 0;
		Client2* m_client = nullptr;
	public:
		ServerHandshakeThread(Client2* client, EventSocket2 socket, const Shared<EventThread2>& handler, size_t maxHandshakeSize, EventLoop2* loop, Log* log, Allocator* allocator)
			: EventThread2(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_handler(handler),
			  m_buffer(allocator),
			  m_maxHandshakeSize(maxHandshakeSize),
			  m_client(client)
		{}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				return m_socket.read(sharedFromThis());
			}
			else if (auto readEvent = dynamic_cast<ReadEvent2*>(event))
			{
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

					auto newConn = unique_from<NewConnection>(m_allocator, m_client);
					if (auto err = m_handler->send(std::move(newConn)))
					{
						m_log->error("failed to send new connection event, {}"_sv, err);
						return stop();
					}
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

	class ImplClient2: public Client2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		Shared<EventThread2> m_handshakeThread;
		Shared<EventThread2> m_readMessageThread;
	public:
		ImplClient2(EventSocket2 socket, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_socket(socket)
		{}

		~ImplClient2() override
		{
			(void)m_handshakeThread->stop();
			(void)m_readMessageThread->stop();
		}

		HumanError startReadingMessages(const Shared<EventThread2>& handler) override
		{
			auto loop = handler->eventLoop();
			m_readMessageThread = loop->startThread<ReadMessageThread>(m_socket, 64ULL * 1024ULL * 1024ULL, loop, m_log, m_allocator);
			return {};
		}

		static Result<Unique<Client2>> accept(EventSocket2 socket, const Shared<EventThread2>& handler, size_t maxHandshakeSize, EventLoop2* loop, Log* log, Allocator* allocator)
		{
			auto res = unique_from<ImplClient2>(allocator, socket, log, allocator);
			res->m_handshakeThread = loop->startThread<ServerHandshakeThread>(res.get(), socket, handler, maxHandshakeSize, loop, log, allocator);
			return res;
		}

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
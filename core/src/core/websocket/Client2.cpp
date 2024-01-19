#include "core/websocket/Client2.h"
#include "core/websocket/MessageParser.h"
#include "core/Url.h"
#include "core/Socket.h"
#include "core/Rand.h"
#include "core/MemoryStream.h"
#include "core/Base64.h"
#include "core/SHA1.h"

namespace core::websocket
{
	class Client2Impl: public Client2, public Reactor
	{
		enum STATE
		{
			STATE_HANDSHAKE,
			STATE_READ_MESSAGE,
			STATE_CLOSED,
			STATE_FAILED,
		};

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		Url m_url;
		Client2Config m_config;
		Shared<EventSource> m_socketSource;
		std::byte m_key[16] = {};
		STATE m_state = STATE_HANDSHAKE;
		Buffer m_readBuffer;
		MessageParser m_messageParser;
		// used to wait until all the writes are done before destroying the connection
		size_t m_pendingWriteBytes = 0;

		HumanError sendHandshake()
		{
			MemoryStream request{m_allocator};

			auto base64Key = Base64::encode(Span<std::byte>{m_key, sizeof(m_key)}, m_allocator);
			auto pathResult = m_url.pathWithQueryAndFragment(m_allocator);
			if (pathResult.isError()) return pathResult.releaseError();
			auto path = pathResult.releaseValue();

			// build the request text
			strf(&request, "GET {} HTTP/1.1\r\n"_sv, path);
			strf(&request, "content-length: 0\r\n"_sv);
			strf(&request, "upgrade: websocket\r\n"_sv);
			strf(&request, "sec-websocket-version: 13\r\n"_sv);
			strf(&request, "connection: upgrade\r\n"_sv);
			strf(&request, "sec-websocket-key: {}\r\n"_sv, base64Key);
			strf(&request, "Host: {}:{}\r\n"_sv, m_url.host(), m_url.port());
			strf(&request, "\r\n"_sv);

			// send the built request
			auto requestAsString = request.releaseString();
			auto loop = m_socketSource->eventLoop();
			if (auto err = loop->write(m_socketSource.get(), nullptr, Span<const std::byte>{(const std::byte*)requestAsString.data(), requestAsString.count()}))
				return err;

			// schedule response read
			m_readBuffer.clear();
			if (auto err = loop->read(m_socketSource.get(), this))
				return err;

			return {};
		}

		HumanError handleHandshakeResponse(StringView response)
		{
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

			m_log->debug("parsed handshake response successfully"_sv);
			return {};
		}

		HumanError writeRaw(Span<const std::byte> bytes, bool waitUntilDone)
		{
			if (bytes.count() == 0)
				return {};

			auto eventLoop = m_socketSource->eventLoop();
			auto reactor = waitUntilDone ? this : nullptr;
			auto err = eventLoop->write(m_socketSource.get(), reactor, bytes);
			if (err) return err;
			if (waitUntilDone)
				m_pendingWriteBytes += bytes.count();
			return {};
		}

		HumanError writeFrameHeader(FrameHeader::OPCODE opcode, size_t payloadLength)
		{
			std::byte buf[10];
			buf[0] = std::byte(128 | opcode);

			if (payloadLength <= 125)
			{
				buf[1] = std::byte(payloadLength);
				if (auto err = writeRaw(Span<const std::byte>{buf, 2}, opcode == FrameHeader::OPCODE_CLOSE)) return err;
			}
			else if (payloadLength <= UINT16_MAX)
			{
				buf[1] = std::byte(126);
				buf[2] = std::byte((payloadLength >> 8) & 0xFF);
				buf[3] = std::byte(payloadLength & 0xFF);
				if (auto err = writeRaw(Span<const std::byte>{buf, 4}, opcode == FrameHeader::OPCODE_CLOSE)) return err;
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
				if (auto err = writeRaw(Span<const std::byte>{buf, 10}, opcode == FrameHeader::OPCODE_CLOSE)) return err;
			}

			return {};
		}

		HumanError writeFrame(FrameHeader::OPCODE opcode, Span<const std::byte> data)
		{
			if (auto err = writeFrameHeader(opcode, data.count())) return err;
			if (auto err = writeRaw(data, opcode == FrameHeader::OPCODE_CLOSE)) return err;
			return {};
		}

		HumanError writeCloseWithCode(uint16_t code, StringView optionalReason)
		{
			uint8_t buf[2];
			buf[0] = (code >> 8) & 0xFF;
			buf[1] = code & 0xFF;
			auto payloadSize = sizeof(buf) + optionalReason.count();
			if (auto err = writeFrameHeader(FrameHeader::OPCODE_CLOSE, payloadSize)) return err;
			if (auto err = writeRaw(Span<const std::byte>{(const std::byte*)buf, sizeof(buf)}, true)) return err;
			if (optionalReason.count() > 0)
				if (auto err = writeRaw(optionalReason, true)) return err;
			return {};
		}

		HumanError onText(const Message& msg)
		{
			m_log->debug("on text"_sv);
			auto text = StringView{msg.payload};
			if (text.isValidUtf8() == false)
				return errf(m_allocator, "invalid utf8 string"_sv);

			if (m_config.onMsg)
			{
				return m_config.onMsg(msg, this);
			}
			return {};
		}

		HumanError onBinary(const Message& msg)
		{
			if (m_config.onMsg)
			{
				return m_config.onMsg(msg, this);
			}
			return {};
		}

		HumanError onClose(const Message& msg)
		{
			m_log->debug("on close"_sv);
			if (m_config.handleClose)
			{
				return m_config.onMsg(msg, this);
			}
			else
			{
				if (msg.payload.count() == 0)
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)}, true);

				// error protocol close payload should have at least 2 byte
				if (msg.payload.count() == 1)
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)}, true);

				auto errorCode = uint16_t(msg.payload[1]) | (uint16_t(msg.payload[0]) << 8);
				if (errorCode < 1000 || errorCode == 1004 || errorCode == 1005 || errorCode == 1006 || (errorCode > 1013  && errorCode < 3000))
				{
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)}, true);
				}

				if (msg.payload.count() == 2)
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)}, true);

				// close payload should be utf8
				auto payload = StringView{msg.payload}.slice(2, msg.payload.count());
				if (payload.isValidUtf8() == false)
				{
					return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)}, true);
				}

				return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)}, true);
			}
		}

		HumanError onPing(const Message& msg)
		{
			if (m_config.handlePing)
			{
				return m_config.onMsg(msg, this);
			}
			else
			{
				if (msg.payload.count() == 0)
				{
					return writeRaw(Span<const std::byte>{(const std::byte *) EMPTY_PONG, sizeof(EMPTY_PONG)}, false);
				}
				else
				{
					return writePong(Span<const std::byte>{msg.payload});
				}
			}
		}

		HumanError onPong(const Message& msg)
		{
			if (m_config.handlePong)
			{
				return m_config.onMsg(msg, this);
			}
			return {};
		}

		HumanError onMsg(const Message& msg)
		{
			switch (msg.type)
			{
			case Message::TYPE_TEXT: return onText(msg);
			case Message::TYPE_BINARY: return onBinary(msg);
			case Message::TYPE_CLOSE: return onClose(msg);
			case Message::TYPE_PING: return onPing(msg);
			case Message::TYPE_PONG: return onPong(msg);
			default:
				assert(false);
				return errf(m_allocator, "Invalid message type"_sv);
			}
		}

		HumanError onConnected()
		{
			if (m_config.onConnected)
				return m_config.onConnected(this);
			return {};
		}

		HumanError onDisconnected()
		{
			if (m_config.onDisconnected)
				return m_config.onDisconnected();
			return {};
		}

		void onHandshake()
		{
			auto eventLoop = m_socketSource->eventLoop();

			auto bufferAsString = StringView{m_readBuffer};
			auto handshakeResponseIndex = bufferAsString.find("\r\n\r\n"_sv);
			if (handshakeResponseIndex != SIZE_MAX)
			{
				// +4 for the \r\n\r\n at the end
				auto handshakeResponse = bufferAsString.slice(0, handshakeResponseIndex + 4);
				auto handshakeErr = handleHandshakeResponse(handshakeResponse);
				if (!handshakeErr)
				{
					m_state = STATE_READ_MESSAGE;

					if (auto err = onConnected())
					{
						sendCloseFrameAndDestroy(1011, err.message());
						return;
					}

					if (m_readBuffer.count() > handshakeResponse.count())
					{
						::memcpy(m_readBuffer.data(), m_readBuffer.data() + handshakeResponse.count(),
								 m_readBuffer.count() - handshakeResponse.count());
						m_readBuffer.resize(m_readBuffer.count() - handshakeResponse.count());
						// process the remaining bytes
						onReadMessage();
					}
					else
					{
						m_readBuffer.clear();
						if (auto err = eventLoop->read(m_socketSource.get(), this))
						{
							sendCloseFrameAndDestroy(1011, "failed to read more bytes from socket"_sv);
						}
					}
				}
				else
				{
					m_log->error("handshake validation failed, {}"_sv, handshakeErr);
					fail();
				}
			}
			else
			{
				if (auto err = eventLoop->read(m_socketSource.get(), this))
				{
					m_log->error("failed to read handshake"_sv);
					fail();
				}
			}
		}

		void onReadMessage()
		{
			auto eventLoop = m_socketSource->eventLoop();

			auto recvBytes = Span<const std::byte>{m_readBuffer};
			while (recvBytes.count() > 0)
			{
				auto parserResult = m_messageParser.consume(recvBytes);
				if (parserResult.isError())
				{
					sendCloseFrameAndDestroy(1002, parserResult.error().message());
					break;
				}
				auto consumedBytes = parserResult.releaseValue();

				// if we didn't consume any bytes we just wait for more bytes
				if (consumedBytes == 0)
				{
					if (auto err = eventLoop->read(m_socketSource.get(), this))
					{
						sendCloseFrameAndDestroy(1011, "failed to read more bytes from socket"_sv);
					}
					break;
				}

				recvBytes = recvBytes.slice(consumedBytes, recvBytes.count() - consumedBytes);
				if (m_messageParser.hasMessage())
				{
					auto msg = m_messageParser.message();
					if (auto err = onMsg(msg))
					{
						sendCloseFrameAndDestroy(1011, err.message());
						break;
					}

					if (msg.type == Message::TYPE_CLOSE)
					{
						close();
						break;
					}
					else
					{
						if (auto err = eventLoop->read(m_socketSource.get(), this))
						{
							sendCloseFrameAndDestroy(1011, "failed to read more bytes from socket"_sv);
							break;
						}
					}
				}
				else
				{
					if (auto err = eventLoop->read(m_socketSource.get(), this))
					{
						sendCloseFrameAndDestroy(1011, "failed to read more bytes from socket"_sv);
					}
					break;
				}
			}

			if (recvBytes.count() == 0)
			{
				m_readBuffer.clear();
			}
			else
			{
				::memcpy(m_readBuffer.data(), recvBytes.data(), recvBytes.count());
				m_readBuffer.resize(recvBytes.count());
			}
		}

		void fail()
		{
			m_state = STATE_FAILED;
			m_socketSource = nullptr;
			m_readBuffer = Buffer{m_allocator};
			if (auto err = onDisconnected())
				m_log->error("{}"_sv, err);
		}

		void destroy()
		{
			if (m_state == STATE_CLOSED && m_pendingWriteBytes == 0)
			{
				m_log->debug("client destroyed"_sv);
				m_socketSource = nullptr;
				m_readBuffer = Buffer{m_allocator};
				if (auto err = onDisconnected())
					m_log->error("{}"_sv, err);
			}
		}

		void close()
		{
			m_state = STATE_CLOSED;
			destroy();
		}

		void sendCloseFrameAndDestroy(uint16_t code, StringView optionalReason)
		{
			if (m_state != STATE_CLOSED)
			{
				(void)writeClose(code, optionalReason);
				m_state = STATE_CLOSED;
			}
			destroy();
		}

	public:
		Client2Impl(Url&& url, Client2Config&& config, const Shared<EventSource>& socketSource, Span<const std::byte> key, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_url(std::move(url)),
			  m_config(std::move(config)),
			  m_socketSource(socketSource),
			  m_readBuffer(allocator),
			  m_messageParser(allocator, 16ULL * 1024ULL * 1024ULL)
		{
			assert(sizeof(m_key) == key.count());
			::memcpy(m_key, key.data(), sizeof(m_key));
		}

		HumanError start() override
		{
			assert(m_state == STATE_HANDSHAKE);
			return sendHandshake();
		}

		void stop() override
		{
			sendCloseFrameAndDestroy(1000, {});
		}

		HumanError writeText(StringView str) override
		{
			return writeFrame(FrameHeader::OPCODE_TEXT, Span<const std::byte>{str});
		}

		HumanError writeBinary(Span<const std::byte> bytes) override
		{
			return writeFrame(FrameHeader::OPCODE_BINARY, bytes);
		}

		HumanError writePing(Span<const std::byte> bytes) override
		{
			return writeFrame(FrameHeader::OPCODE_PING, bytes);
		}

		HumanError writePong(Span<const std::byte> bytes) override
		{
			return writeFrame(FrameHeader::OPCODE_PONG, bytes);
		}

		HumanError writeClose() override
		{
			return writeRaw(Span<const std::byte>{(const std::byte*)CLOSE_NORMAL, sizeof(CLOSE_NORMAL)}, true);
		}

		HumanError writeClose(uint16_t code, StringView optionalReason) override
		{
			return writeCloseWithCode(code, optionalReason);
		}

		void onRead(const ReadEvent* event) override
		{
			if (event->buffer().count() == 0)
			{
				close();
				return;
			}

			if (m_readBuffer.count() + event->buffer().count() > m_config.maxMessageSize)
			{
				sendCloseFrameAndDestroy(1009, "message is too big"_sv);
				return;
			}
			m_readBuffer.push(event->buffer());

			switch (m_state)
			{
			case STATE_HANDSHAKE: onHandshake(); break;
			case STATE_READ_MESSAGE: onReadMessage(); break;
			case STATE_CLOSED:
			case STATE_FAILED:
				break;
			default:
				assert(false);
				break;
			}
		}

		void onWrite(const WriteEvent* event) override
		{
			assert(m_pendingWriteBytes >= event->writtenSize());
			m_pendingWriteBytes -= event->writtenSize();
			destroy();
		}
	};

	Result<Unique<Client2>> Client2::connect(StringView url, Client2Config&& config, EventLoop* loop, Log* log, Allocator* allocator)
	{
		auto parsedUrlResult = core::Url::parse(url, allocator);
		if (parsedUrlResult.isError())
			return parsedUrlResult.releaseError();
		auto parsedUrl = parsedUrlResult.releaseValue();

		auto socket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
		if (socket == nullptr)
			return errf(allocator, "failed to open socket"_sv);

		auto connected = socket->connect(parsedUrl.host(), parsedUrl.port());
		if (connected == false)
			return errf(allocator, "failed to connect to {}"_sv, url);

		auto socketSource = loop->createEventSource(std::move(socket));

		std::byte key[16] = {};
		auto generatedKey = Rand::cryptoRand(Span<std::byte>{key, sizeof(key)});
		if (generatedKey == false)
			return errf(allocator, "failed to generate client key"_sv);

		return unique_from<Client2Impl>(allocator, std::move(parsedUrl), std::move(config), socketSource, Span<const std::byte>{key, sizeof(key)}, log, allocator);
	}
}
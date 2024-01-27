#include "core/websocket/Server.h"
#include "core/websocket/Handshake.h"
#include "core/websocket/MessageParser.h"
#include "core/Hash.h"
#include "core/SHA1.h"
#include "core/Base64.h"

namespace core::websocket
{
	class Conn
	{
		void* m_connHandler = nullptr;
	public:
		Conn(void* connHandler)
			: m_connHandler(connHandler)
		{}

		void* connHandler() const { return m_connHandler; }
	};

	class ServerImpl: public Server
	{
		class ConnHandler: public Reactor
		{
			enum STATE
			{
				STATE_HANDSHAKE,
				STATE_READ_MESSAGE,
				STATE_CLOSED,
			};

			STATE m_state = STATE_HANDSHAKE;
			Allocator* m_allocator = nullptr;
			Log* m_log = nullptr;
			ServerImpl* m_server = nullptr;
			Shared<EventSource> m_socketSource;
			Buffer m_handshakeBuffer;
			size_t m_maxHandshakeSize = 1ULL * 1024ULL;
			Buffer m_messageBuffer;
			MessageParser m_messageParser;
			// used to wait until all the writes are done before destroying the connection
			size_t m_pendingWriteBytes = 0;

			HumanError writeRaw(StringView str, bool waitUntilDone)
			{
				auto eventLoop = m_socketSource->eventLoop();
				Reactor* reactor = nullptr;
				if (waitUntilDone)
				{
					m_pendingWriteBytes += str.count();
					reactor = this;
				}
				auto err = eventLoop->write(m_socketSource.get(), reactor, Span<const std::byte>{str});
				if (err)
				{
					if (waitUntilDone)
						m_pendingWriteBytes -= str.count();
					return err;
				}

				return {};
			}

			HumanError writeRaw(Span<const std::byte> bytes, bool waitUntilDone)
			{
				auto eventLoop = m_socketSource->eventLoop();
				Reactor* reactor = nullptr;
				if (waitUntilDone)
				{
					m_pendingWriteBytes += bytes.count();
					reactor = this;
				}
				auto err = eventLoop->write(m_socketSource.get(), reactor, bytes);
				if (err)
				{
					if (waitUntilDone)
						m_pendingWriteBytes -= bytes.count();
					return err;
				}
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

			void sendHTTPError(StringView str)
			{
				(void)writeRaw(str, true);
			}

			HumanError onTextMsg(const Message& msg, Conn* conn)
			{
				auto text = StringView{msg.payload};
				if (text.isValidUtf8() == false)
					return errf(m_allocator, "invalid utf8 string"_sv);

				if (m_server->m_handler->onMsg)
				{
					m_log->debug("text message: {}"_sv, msg.payload.count());
					return m_server->m_handler->onMsg(msg, m_server, conn);
				}
				return {};
			}

			HumanError onBinaryMsg(const Message& msg, Conn* conn)
			{
				if (m_server->m_handler->onMsg)
				{
					return m_server->m_handler->onMsg(msg, m_server, conn);
				}
				return {};
			}

			HumanError onCloseMsg(const Message& msg, Conn* conn)
			{
				if (m_server->m_handler->handleClose)
				{
					return m_server->m_handler->onMsg(msg, m_server, conn);
				}
				else
				{
					if (msg.payload.count() == 0)
						return writeRaw(Span<const std::byte>{(const std::byte *)CLOSE_NORMAL, sizeof(CLOSE_NORMAL)}, true);

					// error protocol close payload should have at least 2 byte
					if (msg.payload.count() == 1)
						return writeRaw(Span<const std::byte>{(const std::byte*)CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)}, true);

					auto errorCode = uint16_t(msg.payload[1]) | (uint16_t(msg.payload[0]) << 8);
					if (errorCode < 1000 || errorCode == 1004 || errorCode == 1005 || errorCode == 1006 || (errorCode > 1013  && errorCode < 3000))
					{
						return writeRaw(Span<const std::byte>{(const std::byte*) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)}, true);
					}

					if (msg.payload.count() == 2)
						return writeRaw(Span<const std::byte>{(const std::byte*)CLOSE_NORMAL, sizeof(CLOSE_NORMAL)}, true);

					// close payload should be utf8
					auto payload = StringView{msg.payload}.slice(2, msg.payload.count());
					if (payload.isValidUtf8() == false)
						return writeRaw(Span<const std::byte>{(const std::byte*)CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)}, true);

					return writeRaw(Span<const std::byte>{(const std::byte*)CLOSE_NORMAL, sizeof(CLOSE_NORMAL)}, true);
				}
			}

			HumanError onPingMsg(const Message& msg, Conn* conn)
			{
				if (m_server->m_handler->handlePing)
				{
					return m_server->m_handler->onMsg(msg, m_server, conn);
				}
				else
				{
					if (msg.payload.count() == 0)
					{
						return writeRaw(Span<const std::byte>{(const std::byte*)EMPTY_PONG, sizeof(EMPTY_PONG)}, false);
					}
					else
					{
						return writePong(Span<const std::byte>{msg.payload});
					}
				}
			}

			HumanError onPongMsg(const Message& msg, Conn* conn)
			{
				if (m_server->m_handler->handlePong)
				{
					return m_server->m_handler->onMsg(msg, m_server, conn);
				}
				return {};
			}

			HumanError onMsg(const Message& msg, Conn* conn)
			{
				switch (msg.type)
				{
				case Message::TYPE_TEXT: return onTextMsg(msg, conn);
				case Message::TYPE_BINARY: return onBinaryMsg(msg, conn);
				case Message::TYPE_CLOSE: return onCloseMsg(msg, conn);
				case Message::TYPE_PING: return onPingMsg(msg, conn);
				case Message::TYPE_PONG: return onPongMsg(msg, conn);
				default:
					assert(false);
					return errf(m_allocator, "invalid message type"_sv);
				}
			}

			void removeFromServer()
			{
				if (m_state == STATE_CLOSED && m_pendingWriteBytes == 0)
					m_server->removeConn(this);
			}

			void closeAndDestroy()
			{
				m_state = STATE_CLOSED;
				removeFromServer();
			}

			void sendCloseFrameAndDestroy(uint16_t code, StringView optionalReason)
			{
				if (m_state != STATE_CLOSED)
				{
					(void)writeCloseWithCode(code, optionalReason);
					m_state = STATE_CLOSED;
				}
				removeFromServer();
			}

		public:
			static Result<Unique<ConnHandler>> create(Unique<Socket> socket, EventLoop* loop, ServerImpl* server, size_t maxHandshakeSize, Log* log, Allocator* allocator)
			{
				auto socketSource = loop->createEventSource(std::move(socket));
				if (socketSource == nullptr)
					return errf(allocator, "failed to convert connection socket to event loop source"_sv);

				auto res = unique_from<ConnHandler>(allocator, std::move(socketSource), server, maxHandshakeSize, log, allocator);
				auto err = loop->read(res->m_socketSource.get(), res.get());
				if (err)
					return err;
				return res;
			}

			ConnHandler(const Shared<EventSource>& socketSource, ServerImpl* server, size_t maxHanshakeSize, Log* log, Allocator* allocator)
				: m_allocator(allocator),
				  m_log(log),
				  m_server(server),
				  m_socketSource(socketSource),
				  m_handshakeBuffer(allocator),
				  m_maxHandshakeSize(maxHanshakeSize),
				  m_messageBuffer(allocator),
				  m_messageParser(allocator, 64ULL * 1024ULL * 1024ULL)
			{}

			HumanError writeText(StringView str)
			{
				return writeFrame(FrameHeader::OPCODE_TEXT, Span<const std::byte>{str});
			}

			HumanError writeBinary(Span<const std::byte> bytes)
			{
				return writeFrame(FrameHeader::OPCODE_BINARY, bytes);
			}

			HumanError writePing(Span<const std::byte> bytes)
			{
				return writeFrame(FrameHeader::OPCODE_PING, bytes);
			}

			HumanError writeCloseNormally()
			{
				return writeRaw(Span<const std::byte>{(const std::byte *) CLOSE_NORMAL, sizeof(CLOSE_NORMAL)}, true);
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

			HumanError writePong(Span<const std::byte> bytes)
			{
				return writeFrame(FrameHeader::OPCODE_PONG, bytes);
			}

			void onRead(const ReadEvent* event)
			{
				auto eventLoop = event->eventLoop();

				// zero read means connection is closed
				if (event->buffer().count() == 0)
				{
					closeAndDestroy();
					return;
				}

				switch (m_state)
				{
				case STATE_HANDSHAKE:
				{
					auto totalHandshakeBuffer = m_handshakeBuffer.count() + event->buffer().count();
					if (totalHandshakeBuffer > m_maxHandshakeSize)
					{
						sendHTTPError(R"(HTTP/1.1 400 Invalid\r\nerror: too large\r\ncontent-length: 0\r\n\r\n)"_sv);
						closeAndDestroy();
						return;
					}

					m_handshakeBuffer.push(event->buffer());

					auto handshakeString = StringView{m_handshakeBuffer};
					if (handshakeString.endsWith("\r\n\r\n"_sv))
					{
						auto handshakeResult = Handshake::parse(handshakeString, m_allocator);
						if (handshakeResult.isError())
						{
							sendHTTPError(R"(HTTP/1.1 400 Invalid\r\nerror: failed to parse handshake\r\ncontent-length: 0\r\n\r\n)"_sv);
							closeAndDestroy();
							return;
						}
						auto handshake = handshakeResult.releaseValue();
						m_handshakeBuffer = Buffer{m_allocator};

						constexpr static const char* REPLY = "HTTP/1.1 101 Switching Protocols\r\n"
							"Upgrade: websocket\r\n"
							"Connection: Upgrade\r\n"
							"Sec-WebSocket-Accept: {}\r\n"
							"\r\n";
						auto concatKey = strf(m_allocator, "{}{}"_sv, handshake.key(), "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv);
						auto sha1 = SHA1::hash(concatKey);
						auto base64 = Base64::encode(sha1.asBytes(), m_allocator);
						auto reply = strf(m_allocator, StringView{REPLY, strlen(REPLY)}, base64);
						if (auto err = writeRaw(reply, true))
						{
							m_log->error("failed to send handshake reply, {}"_sv, err);
							closeAndDestroy();
							return;
						}

						m_state = STATE_READ_MESSAGE;
						if (auto err = eventLoop->read(m_socketSource.get(), this))
						{
							m_log->error("failed to schedule read operation"_sv);
							closeAndDestroy();
							return;
						}
					}
					break;
				}
				case STATE_READ_MESSAGE:
				{
					m_messageBuffer.push(event->buffer());

					auto bytes = Span<const std::byte>{m_messageBuffer};
					while (bytes.count() > 0)
					{
						auto parserResult = m_messageParser.consume(bytes);
						if (parserResult.isError())
						{
							// NOTE: we can fail to close the connection cleanly here
							sendCloseFrameAndDestroy(1002, parserResult.error().message());
							return;
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
							Conn conn{this};
							if (auto err = onMsg(msg, &conn))
							{
								// NOTE: we can fail to close the connection cleanly here
								sendCloseFrameAndDestroy(1011, err.message());
								return;
							}

							// check if this is a close message
							if (msg.type == Message::TYPE_CLOSE)
							{
								closeAndDestroy();
								return;
							}
						}
					}

					if (bytes.count() == 0)
					{
						m_messageBuffer.clear();
					}
					else
					{
						::memmove(m_messageBuffer.data(), bytes.data(), bytes.count());
						m_messageBuffer.resize(bytes.count());
					}

					if (auto err = eventLoop->read(m_socketSource.get(), this))
					{
						// NOTE: we can fail to close the connection cleanly here
						sendCloseFrameAndDestroy(1011, err.message());
						return;
					}
					break;
				}
				default:
					assert(false);
					break;
				}
			}

			void onWrite(const WriteEvent* event) override
			{
				assert(m_pendingWriteBytes >= event->writtenSize());
				m_pendingWriteBytes -= event->writtenSize();
				removeFromServer();
			}
		};

		class AcceptHandler: public Reactor
		{
			Allocator* m_allocator = nullptr;
			Log* m_log = nullptr;
			ServerImpl* m_server = nullptr;
			Shared<EventSource> m_socketSource;
		public:
			static Result<Unique<AcceptHandler>> create(StringView host, StringView port, EventLoop* loop, ServerImpl* server, Log* log, Allocator* allocator)
			{
				auto acceptSocket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
				if (acceptSocket == nullptr)
					return errf(allocator, "failed to open accept socket"_sv);

				auto ok = acceptSocket->bind(host, port);
				if (ok == false)
					return errf(allocator, "failed to bind accept socket to {}:{}"_sv, host, port);

				ok = acceptSocket->listen();
				if (ok == false)
					return errf(allocator, "failed to listen to {}:{}"_sv, host, port);

				auto socketSource = loop->createEventSource(std::move(acceptSocket));
				if (socketSource == nullptr)
					return errf(allocator, "failed to convert socket to event loop source"_sv);

				auto res = unique_from<AcceptHandler>(allocator, socketSource, server, log, allocator);
				auto err = loop->accept(res->m_socketSource.get(), res.get());
				if (err)
					return err;

				return res;
			}

			AcceptHandler(const Shared<EventSource>& socketSource, ServerImpl* server, Log* log, Allocator* allocator)
				: m_allocator(allocator),
				  m_log(log),
				  m_server(server),
				  m_socketSource(socketSource)
			{}

			void onAccept(AcceptEvent* event) override
			{
				auto socket = event->releaseSocket();
				auto eventLoop = event->eventLoop();
				auto connResult = ConnHandler::create(std::move(socket), eventLoop, m_server, m_server->maxHandshakeSize, m_log, m_allocator);
				if (connResult.isError())
				{
					m_log->error("failed to create connection, {}"_sv, connResult.error());
					return;
				}
				m_server->addConn(connResult.releaseValue());

				if (auto err = eventLoop->accept(m_socketSource.get(), this))
				{
					m_log->error("failed to schedule accept operation, {}"_sv, err);
					return;
				}
			}
		};

		void addConn(Unique<ConnHandler> conn)
		{
			m_connections.insert(conn.get(), std::move(conn));
		}

		void removeConn(ConnHandler* conn)
		{
			m_connections.remove(conn);
		}

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		Unique<AcceptHandler> m_acceptHandler;
		Map<ConnHandler*, Unique<ConnHandler>> m_connections;
		size_t maxHandshakeSize = 1ULL * 1024ULL;
		ServerHandler* m_handler = nullptr;
	public:
		ServerImpl(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_connections(allocator)
		{}

		HumanError start(ServerConfig config, EventLoop* loop, ServerHandler* handler) override
		{
			m_handler = handler;
			maxHandshakeSize = config.maxHandshakeSize;
			auto acceptHandlerResult = AcceptHandler::create(config.host, config.port, loop, this, m_log, m_allocator);
			if (acceptHandlerResult.isError())
				return acceptHandlerResult.releaseError();
			m_acceptHandler = acceptHandlerResult.releaseValue();
			return {};
		}

		void stop() override
		{
			m_acceptHandler = nullptr;
			for (auto& [conn, _]: m_connections)
				(void)conn->writeCloseNormally();
			m_connections.clear();
		}

		HumanError writeText(Conn* conn, StringView str) override
		{
			auto connHandler = (ConnHandler*)conn->connHandler();
			return connHandler->writeText(str);
		}

		HumanError writeBinary(Conn* conn, Span<const std::byte> bytes) override
		{
			auto connHandler = (ConnHandler*)conn->connHandler();
			return connHandler->writeBinary(bytes);
		}

		HumanError writePing(Conn* conn, Span<const std::byte> bytes) override
		{
			auto connHandler = (ConnHandler*)conn->connHandler();
			return connHandler->writePing(bytes);
		}

		HumanError writePong(Conn* conn, Span<const std::byte> bytes) override
		{
			auto connHandler = (ConnHandler*)conn->connHandler();
			return connHandler->writePong(bytes);
		}

		HumanError writeClose(Conn* conn) override
		{
			auto connHandler = (ConnHandler*)conn->connHandler();
			return connHandler->writeCloseNormally();
		}

		HumanError writeClose(Conn* conn, uint16_t code, StringView optionalReason) override
		{
			auto connHandler = (ConnHandler*)conn->connHandler();
			return connHandler->writeCloseWithCode(code, optionalReason);
		}
	};

	Result<Unique<Server>> Server::create(Log *log, Allocator *allocator)
	{
		return unique_from<ServerImpl>(allocator, log, allocator);
	}
}
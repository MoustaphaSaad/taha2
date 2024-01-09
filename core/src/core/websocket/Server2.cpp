#include "core/websocket/Server2.h"
#include "core/websocket/Handshake.h"
#include "core/websocket/MessageParser.h"
#include "core/Hash.h"
#include "core/SHA1.h"
#include "core/Base64.h"

namespace core::websocket
{
	class Server2Impl: public Server2
	{
		class Conn: public Reactor
		{
			enum STATE
			{
				STATE_HANDSHAKE,
				STATE_READ_MESSAGE,
			};

			STATE m_state = STATE_HANDSHAKE;
			Allocator* m_allocator = nullptr;
			Log* m_log = nullptr;
			Server2Impl* m_server = nullptr;
			Unique<EventSource> m_socketSource;
			Buffer m_handshakeBuffer;
			size_t m_maxHandshakeSize = 1ULL * 1024ULL;
			Buffer m_messageBuffer;
			MessageParser m_messageParser;

			HumanError writeRaw(StringView str)
			{
				auto eventLoop = m_socketSource->eventLoop();
				return eventLoop->write(m_socketSource.get(), nullptr, Span<const std::byte>{str});
			}

			HumanError writeRaw(Span<const std::byte> bytes)
			{
				auto eventLoop = m_socketSource->eventLoop();
				return eventLoop->write(m_socketSource.get(), nullptr, bytes);
			}

			HumanError writeFrameHeader(FrameHeader::OPCODE opcode, size_t payloadLength)
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

			HumanError writeFrame(FrameHeader::OPCODE opcode, Span<const std::byte> data)
			{
				if (auto err = writeFrameHeader(opcode, data.count())) return err;
				if (auto err = writeRaw(data)) return err;
				return {};
			}

			HumanError writePong(Span<const std::byte> bytes)
			{
				return writeFrame(FrameHeader::OPCODE_PONG, bytes);
			}

			void sendHTTPError(StringView str)
			{
				(void)writeRaw(str);
			}

			HumanError onTextMsg(const Message& msg)
			{
				auto text = StringView{msg.payload};
				if (text.isValidUtf8() == false)
					return errf(m_allocator, "invalid utf8 string"_sv);

				if (m_server->m_handler->onMsg)
				{
					return m_server->m_handler->onMsg(msg);
				}
				return {};
			}

			HumanError onBinaryMsg(const Message& msg)
			{
				if (m_server->m_handler->onMsg)
				{
					return m_server->m_handler->onMsg(msg);
				}
				return {};
			}

			HumanError onCloseMsg(const Message& msg)
			{
				if (m_server->m_handler->handleClose)
				{
					return m_server->m_handler->onMsg(msg);
				}
				else
				{
					if (msg.payload.count() == 0)
						return writeRaw(Span<const std::byte>{(const std::byte *)CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});

					// error protocol close payload should have at least 2 byte
					if (msg.payload.count() == 1)
						return writeRaw(Span<const std::byte>{(const std::byte*)CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)});

					auto errorCode = uint16_t(msg.payload[1]) | (uint16_t(msg.payload[0]) << 8);
					if (errorCode < 1000 || errorCode == 1004 || errorCode == 1005 || errorCode == 1006 || (errorCode > 1013  && errorCode < 3000))
					{
						return writeRaw(Span<const std::byte>{(const std::byte*) CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)});
					}

					if (msg.payload.count() == 2)
						return writeRaw(Span<const std::byte>{(const std::byte*)CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});

					// close payload should be utf8
					auto payload = StringView{msg.payload}.slice(2, msg.payload.count());
					if (payload.isValidUtf8() == false)
						return writeRaw(Span<const std::byte>{(const std::byte*)CLOSE_PROTOCOL_ERROR, sizeof(CLOSE_PROTOCOL_ERROR)});

					return writeRaw(Span<const std::byte>{(const std::byte*)CLOSE_NORMAL, sizeof(CLOSE_NORMAL)});
				}
			}

			HumanError onPingMsg(const Message& msg)
			{
				if (m_server->m_handler->handlePing)
				{
					return m_server->m_handler->onMsg(msg);
				}
				else
				{
					if (msg.payload.count() == 0)
					{
						return writeRaw(Span<const std::byte>{(const std::byte*)EMPTY_PONG, sizeof(EMPTY_PONG)});
					}
					else
					{
						return writePong(Span<const std::byte>{msg.payload});
					}
				}
			}

			HumanError onPongMsg(const Message& msg)
			{
				if (m_server->m_handler->handlePong)
				{
					return m_server->m_handler->onMsg(msg);
				}
				return {};
			}

			HumanError onMsg(const Message& msg)
			{
				switch (msg.type)
				{
				case Message::TYPE_TEXT: return onTextMsg(msg);
				case Message::TYPE_BINARY: return onBinaryMsg(msg);
				case Message::TYPE_CLOSE: return onCloseMsg(msg);
				case Message::TYPE_PING: return onPingMsg(msg);
				case Message::TYPE_PONG: return onPongMsg(msg);
				default:
					assert(false);
					return errf(m_allocator, "invalid message type"_sv);
				}
			}

		public:
			static Result<Unique<Conn>> create(Unique<Socket> socket, EventLoop* loop, Server2Impl* server, size_t maxHandshakeSize, Log* log, Allocator* allocator)
			{
				auto socketSource = loop->createEventSource(std::move(socket));
				if (socketSource == nullptr)
					return errf(allocator, "failed to convert connection socket to event loop source"_sv);

				auto res = unique_from<Conn>(allocator, std::move(socketSource), server, maxHandshakeSize, log, allocator);
				auto err = loop->read(res->m_socketSource.get(), res.get());
				if (err)
					return err;
				return res;
			}

			Conn(Unique<EventSource> socketSource, Server2Impl* server, size_t maxHanshakeSize, Log* log, Allocator* allocator)
				: m_allocator(allocator),
				  m_log(log),
				  m_server(server),
				  m_socketSource(std::move(socketSource)),
				  m_handshakeBuffer(allocator),
				  m_maxHandshakeSize(maxHanshakeSize),
				  m_messageBuffer(allocator),
				  m_messageParser(allocator, 64ULL * 1024ULL * 1024ULL)
			{}

			void onRead(const ReadEvent* event)
			{
				auto eventLoop = event->eventLoop();

				switch (m_state)
				{
				case STATE_HANDSHAKE:
				{
					auto totalHandshakeBuffer = m_handshakeBuffer.count() + event->buffer().count();
					if (totalHandshakeBuffer > m_maxHandshakeSize)
					{
						sendHTTPError(R"(HTTP/1.1 400 Invalid\r\nerror: too large\r\ncontent-length: 0\r\n\r\n)"_sv);
						m_server->removeConn(this);
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
							m_server->removeConn(this);
							return;
						}
						auto handshake = handshakeResult.releaseValue();
						m_handshakeBuffer = Buffer{m_allocator};

						m_log->debug("handshake: {}"_sv, handshake.key());

						constexpr static const char* REPLY = "HTTP/1.1 101 Switching Protocols\r\n"
							"Upgrade: websocket\r\n"
							"Connection: Upgrade\r\n"
							"Sec-WebSocket-Accept: {}\r\n"
							"\r\n";
						auto concatKey = strf(m_allocator, "{}{}"_sv, handshake.key(), "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv);
						auto sha1 = SHA1::hash(concatKey);
						auto base64 = Base64::encode(sha1.asBytes(), m_allocator);
						auto reply = strf(m_allocator, StringView{REPLY, strlen(REPLY)}, base64);
						m_log->debug("response: {}"_sv, reply);
						if (auto err = writeRaw(reply))
						{
							m_log->error("failed to send handshake reply, {}"_sv, err);
							m_server->removeConn(this);
							return;
						}

						m_state = STATE_READ_MESSAGE;
						if (auto err = eventLoop->read(m_socketSource.get(), this))
						{
							m_log->error("failed to schedule read operation"_sv);
							m_server->removeConn(this);
							return;
						}
					}
					break;
				}
				case STATE_READ_MESSAGE:
				{
					m_messageBuffer.push(event->buffer());

					auto bytes = Span<const std::byte>{m_messageBuffer};
					HumanError error{};
					while (bytes.count() > 0)
					{
						auto parserResult = m_messageParser.consume(bytes);
						if (parserResult.isError())
						{
							error = parserResult.releaseError();
							break;
						}
						auto consumedBytes = parserResult.value();

						// if we didn't consume any bytes we just wait for more bytes
						if (consumedBytes == 0)
							break;

						bytes = bytes.slice(consumedBytes, bytes.count() - consumedBytes);

						if (m_messageParser.hasMessage())
						{
							auto msg = m_messageParser.message();
							m_log->debug("type: {}, payload: {}"_sv, (int)msg.type, StringView{msg.payload});
							if (auto err = onMsg(msg))
							{
								// TODO: close with error
								m_server->removeConn(this);
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
						::memcpy(m_messageBuffer.data(), bytes.data(), bytes.count());
						m_messageBuffer.resize(bytes.count());
					}

					if (error)
					{
						// TODO: do something about this error
						// sendError("failed to parse read message"_sv);
						m_server->removeConn(this);
						return;
					}

					if (auto err = eventLoop->read(m_socketSource.get(), this))
					{
						// sendError("failed to schedule read"_sv);
						m_server->removeConn(this);
						return;
					}
					break;
				}
				default:
					assert(false);
					break;
				}
			}
		};

		class AcceptHandler: public Reactor
		{
			Allocator* m_allocator = nullptr;
			Log* m_log = nullptr;
			Server2Impl* m_server = nullptr;
			Unique<EventSource> m_socketSource;
		public:
			static Result<Unique<AcceptHandler>> create(StringView host, StringView port, EventLoop* loop, Server2Impl* server, Log* log, Allocator* allocator)
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

				auto res = unique_from<AcceptHandler>(allocator, std::move(socketSource), server, log, allocator);
				auto err = loop->accept(res->m_socketSource.get(), res.get());
				if (err)
					return err;

				return res;
			}

			AcceptHandler(Unique<EventSource> socketSource, Server2Impl* server, Log* log, Allocator* allocator)
				: m_allocator(allocator),
				  m_log(log),
				  m_server(server),
				  m_socketSource(std::move(socketSource))
			{}

			void onAccept(AcceptEvent* event) override
			{
				auto socket = event->releaseSocket();
				auto eventLoop = event->eventLoop();
				auto connResult = Conn::create(std::move(socket), eventLoop, m_server, m_server->maxHandshakeSize, m_log, m_allocator);
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

		void addConn(Unique<Conn> conn)
		{
			m_connections.insert(conn.get(), std::move(conn));
		}

		void removeConn(Conn* conn)
		{
			m_connections.remove(conn);
		}

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		Unique<AcceptHandler> m_acceptHandler;
		Map<Conn*, Unique<Conn>> m_connections;
		size_t maxHandshakeSize = 1ULL * 1024ULL;
		ServerHandler2* m_handler = nullptr;
	public:
		Server2Impl(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_connections(allocator)
		{}

		HumanError start(ServerConfig2 config, EventLoop* loop, ServerHandler2* handler) override
		{
			m_handler = handler;
			maxHandshakeSize = config.maxHandshakeSize;
			auto acceptHandlerResult = AcceptHandler::create(config.host, config.port, loop, this, m_log, m_allocator);
			if (acceptHandlerResult.isError())
				return acceptHandlerResult.releaseError();
			m_acceptHandler = acceptHandlerResult.releaseValue();
			return {};
		}
	};

	Result<Unique<Server2>> Server2::create(Log *log, Allocator *allocator)
	{
		return unique_from<Server2Impl>(allocator, log, allocator);
	}
}
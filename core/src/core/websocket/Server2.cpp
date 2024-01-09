#include "core/websocket/Server2.h"
#include "core/websocket/Handshake.h"
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

			void error(StringView str)
			{
				auto eventLoop = m_socketSource->eventLoop();
				(void)eventLoop->write(m_socketSource.get(), nullptr, Span<const std::byte>{str});
			}

			HumanError write(StringView str)
			{
				auto eventLoop = m_socketSource->eventLoop();
				return eventLoop->write(m_socketSource.get(), nullptr, Span<const std::byte>{str});
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
				  m_maxHandshakeSize(maxHanshakeSize)
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
						error(R"(HTTP/1.1 400 Invalid\r\nerror: too large\r\ncontent-length: 0\r\n\r\n)"_sv);
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
							error(R"(HTTP/1.1 400 Invalid\r\nerror: failed to parse handshake\r\ncontent-length: 0\r\n\r\n)"_sv);
							m_server->removeConn(this);
							return;
						}
						auto handshake = handshakeResult.releaseValue();

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
						if (auto err = write(reply))
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
				// TODO: handle new connections
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
	public:
		Server2Impl(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_connections(allocator)
		{}

		HumanError start(ServerConfig2 config, EventLoop* loop) override
		{
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
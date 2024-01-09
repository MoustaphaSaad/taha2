#include "core/websocket/Server2.h"
#include "core/Hash.h"

namespace core::websocket
{
	class Server2Impl: public Server2
	{
		class Conn: public Reactor
		{
			Log* m_log = nullptr;
			Server2Impl* m_server = nullptr;
			Unique<EventSource> m_socketSource;
		public:
			static Result<Unique<Conn>> create(Unique<Socket> socket, EventLoop* loop, Server2Impl* server, Log* log, Allocator* allocator)
			{
				auto socketSource = loop->createEventSource(std::move(socket));
				if (socketSource == nullptr)
					return errf(allocator, "failed to convert connection socket to event loop source"_sv);

				auto res = unique_from<Conn>(allocator, std::move(socketSource), server, log);
				auto err = loop->read(res->m_socketSource.get(), res.get());
				if (err)
					return err;
				return res;
			}

			Conn(Unique<EventSource> socketSource, Server2Impl* server, Log* log)
				: m_log(log),
				  m_server(server),
				  m_socketSource(std::move(socketSource))
			{}

			void onRead(const ReadEvent* event)
			{
				m_log->debug("connection read: {}"_sv, StringView{event->buffer()});
				// schedule next read
				auto eventLoop = event->eventLoop();
				if (auto err = eventLoop->read(m_socketSource.get(), this))
				{
					m_log->error("failed to schedule read operation for connection"_sv);
					m_server->removeConn(this);
					return;
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
				auto connResult = Conn::create(std::move(socket), eventLoop, m_server, m_log, m_allocator);
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
	public:
		Server2Impl(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_connections(allocator)
		{}

		HumanError start(ServerConfig2 config, EventLoop* loop) override
		{
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
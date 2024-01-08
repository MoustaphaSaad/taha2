#include "core/websocket/XXServer.h"
#include "core/Hash.h"

namespace core::websocket
{
	class XXConn: public Reactor
	{
	public:
		enum STATE
		{
			STATE_HANDSHAKE,
			STATE_READ_MESSAGE,
		};

		static Result<Unique<XXConn>> create(Unique<Socket> socket, EventLoop* loop, Allocator* allocator)
		{
			auto socketSource = loop->createEventSource(std::move(socket));
			if (socketSource == nullptr)
				return errf(allocator, "failed to convert connection socket into event source"_sv);

			auto res = unique_from<XXConn>(allocator, std::move(socketSource));
			auto err = loop->read(res->m_socketSource.get(), res.get());
			if (err)
				return err;

			return res;
		}

		explicit XXConn(Unique<EventSource> socketSource)
			: m_socketSource(std::move(socketSource))
		{}

		void onRead(const ReadEvent* event) override
		{
			switch (m_state)
			{
			case STATE_HANDSHAKE:
				// TODO: handle handshake too long error

				break;
			case STATE_READ_MESSAGE:
				break;
			default:
				// TODO: handle this in a better way
				assert(false);
				break;
			}
		}

	private:
		STATE m_state = STATE_HANDSHAKE;
		Unique<EventSource> m_socketSource;
	};

	class XXServerImpl: public XXServer
	{
		class Acceptor: public Reactor
		{
			Unique<EventSource> m_acceptSource;
			XXServerImpl* m_server = nullptr;
		public:
			static Result<Unique<Acceptor>> create(StringView ip, StringView port, EventLoop* loop, XXServerImpl* server, Allocator* allocator)
			{
				auto acceptSocket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
				if (acceptSocket == nullptr)
					return errf(allocator, "failed to create accept socket"_sv);

				// TODO: add ip to bind
				auto ok = acceptSocket->bind(port);
				if (ok == false)
					return errf(allocator, "failed to bind accept socket"_sv);

				ok = acceptSocket->listen();
				if (ok == false)
					return errf(allocator, "failed to listen to accept socket"_sv);

				auto acceptSource = loop->createEventSource(std::move(acceptSocket));
				if (acceptSource == nullptr)
					return errf(allocator, "failed to convert accept socket into an event source"_sv);

				auto res = unique_from<Acceptor>(allocator, std::move(acceptSource), server);

				// schedule first accept
				auto err = loop->accept(res->m_acceptSource.get(), res.get());
				if (err) return err;

				return res;
			}

			explicit Acceptor(Unique<EventSource> acceptSource, XXServerImpl* server)
				: m_acceptSource(std::move(acceptSource)),
				  m_server(server)
			{}

			void onAccept(core::AcceptEvent* event) override
			{
				auto socket = event->releaseSocket();
				auto connResult = XXConn::create(std::move(socket), m_acceptSource->eventLoop(), m_server->m_allocator);
				if (connResult.isError())
				{
					// TODO: consider handling this in a better way
					m_server->m_log->error("failed to create connection for the accepted socket, {}"_sv, connResult.error());
					return;
				}
				auto conn = connResult.releaseValue();

				m_server->m_connections.insert(conn.get(), std::move(conn));
			}
		};

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		Unique<Acceptor> m_acceptor;
		Map<XXConn*, Unique<XXConn>> m_connections;
	public:
		XXServerImpl(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_connections(allocator)
		{}

		HumanError start(EventLoop* loop, StringView ip, StringView port) override
		{
			auto acceptorResult = Acceptor::create(ip, port, loop, this, m_allocator);
			if (acceptorResult.isError())
				return acceptorResult.releaseError();
			m_acceptor = acceptorResult.releaseValue();
			return {};
		}
	};

	Result<Unique<XXServer>> XXServer::open(Log *log, Allocator *allocator)
	{
		return unique_from<XXServerImpl>(allocator, log, allocator);
	}
}
#include "core/websocket/Server3.h"
#include "core/websocket/Client3.h"

namespace core::websocket
{
	class AcceptThread3: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		Server3* m_server = nullptr;
	public:
		AcceptThread3(Server3* server, EventSocket2 socket, EventLoop2* loop, Log* log, Allocator* allocator)
			: EventThread2(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_server(server)
		{}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				return m_socket.accept(sharedFromThis());
			}
			else if (auto acceptEvent = dynamic_cast<AcceptEvent2*>(event))
			{
				m_server->clientConnected(acceptEvent->releaseSocket());
				return m_socket.accept(sharedFromThis());
			}
			return {};
		}
	};

	Server3::ClientSet::ClientSet(Allocator *allocator)
		: m_mutex(allocator),
		  m_clients(allocator)
	{}

	void Server3::ClientSet::push(const Shared<Client3>& client)
	{
		auto lock = Lock<Mutex>::lock(m_mutex);
		m_clients.insert(client);
	}

	void Server3::ClientSet::pop(const Shared<Client3>& client)
	{
		auto lock = Lock<Mutex>::lock(m_mutex);
		m_clients.remove(client);
	}

	void Server3::clientConnected(Unique<Socket> socket)
	{
		auto loop = m_acceptThread->eventLoop()->next();
		auto socketResult = loop->registerSocket(std::move(socket));
		if (socketResult.isError())
		{
			m_log->error("failed to register client socket, {}"_sv, socketResult.releaseError());
			return;
		}

		auto client = Client3::acceptFromServer(
			this,
			loop,
			socketResult.releaseValue(),
			m_maxHandshakeSize,
			m_maxMessageSize,
			m_log,
			m_allocator
		);
		m_clientSet.push(std::move(client));
	}

	void Server3::clientHandshakeDone(const Shared<Client3>& client)
	{
		auto newConn = unique_from<NewConnection3>(m_allocator, client);
		(void) m_handler->send(std::move(newConn));
	}

	void Server3::clientClosed(const Shared<Client3>& client)
	{
		m_clientSet.pop(client);
	}

	Server3::Server3(Log *log, Allocator *allocator)
		: m_allocator(allocator),
		  m_log(log),
		  m_clientSet(allocator)
	{}

	Unique<Server3> Server3::create(Log* log, Allocator* allocator)
	{
		return unique_from<Server3>(allocator, log, allocator);
	}

	HumanError Server3::start(const ServerConfig3& config, EventLoop2* loop)
	{
		auto socket = Socket::open(m_allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
		if (socket == nullptr)
			return errf(m_allocator, "failed to open accept socket"_sv);

		auto ok = socket->bind(config.host, config.port);
		if (ok == false)
			return errf(m_allocator, "failed to bind socket"_sv);

		ok = socket->listen();
		if (ok == false)
			return errf(m_allocator, "failed to listen socket"_sv);

		auto eventSocketResult = loop->registerSocket(std::move(socket));
		if (eventSocketResult.isError())
			return eventSocketResult.releaseError();
		auto eventSocket = eventSocketResult.releaseValue();

		m_handler = config.handler;
		m_maxHandshakeSize = config.maxHandshakeSize;
		m_maxMessageSize = config.maxMessageSize;
		m_acceptThread = loop->startThread<AcceptThread3>(this, eventSocket, loop, m_log, m_allocator);
		return {};
	}
}
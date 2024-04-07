#include "core/websocket/Server.h"
#include "core/websocket/Client.h"

namespace core::websocket
{
	class AcceptThread: public EventThread
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket m_socket;
		Server* m_server = nullptr;
	public:
		AcceptThread(Server* server, EventSocket socket, EventLoop* loop, Log* log, Allocator* allocator)
			: EventThread(loop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_server(server)
		{}

		HumanError handle(Event* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent*>(event))
			{
				return m_socket.accept(sharedFromThis());
			}
			else if (auto acceptEvent = dynamic_cast<AcceptEvent*>(event))
			{
				m_server->clientConnected(acceptEvent->releaseSocket());
				return m_socket.accept(sharedFromThis());
			}
			return {};
		}
	};

	Server::ClientSet::ClientSet(Allocator *allocator)
		: m_mutex(allocator),
		  m_clients(allocator)
	{}

	void Server::ClientSet::push(const Shared<Client>& client)
	{
		auto lock = lockGuard(m_mutex);
		m_clients.insert(client);
	}

	void Server::ClientSet::pop(const Shared<Client>& client)
	{
		auto lock = lockGuard(m_mutex);
		m_clients.remove(client);
	}

	void Server::clientConnected(Unique<Socket> socket)
	{
		auto loop = m_acceptThread->eventLoop()->next();
		auto socketResult = loop->registerSocket(std::move(socket));
		if (socketResult.isError())
		{
			m_log->error("failed to register client socket, {}"_sv, socketResult.releaseError());
			return;
		}

		auto client = Client::acceptFromServer(
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

	void Server::clientHandshakeDone(const Shared<Client>& client)
	{
		auto newConn = unique_from<NewConnection>(m_allocator, client);
		(void) m_handler->send(std::move(newConn));
	}

	void Server::clientClosed(const Shared<Client>& client)
	{
		m_clientSet.pop(client);
	}

	Server::Server(Log *log, Allocator *allocator)
		: m_allocator(allocator),
		  m_log(log),
		  m_clientSet(allocator)
	{}

	Unique<Server> Server::create(Log* log, Allocator* allocator)
	{
		return unique_from<Server>(allocator, log, allocator);
	}

	HumanError Server::start(const ServerConfig& config, EventLoop* loop)
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
		m_acceptThread = loop->startThread<AcceptThread>(this, eventSocket, loop, m_log, m_allocator);
		return {};
	}
}
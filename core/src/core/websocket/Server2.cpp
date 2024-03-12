#include "core/websocket/Server2.h"
#include "core/websocket/Handshake.h"
#include "core/websocket/MessageParser.h"
#include "core/websocket/Internal.h"
#include "core/SHA1.h"
#include "core/Base64.h"
#include "core/Rand.h"
#include "core/Mutex.h"
#include "core/Hash.h"

namespace core::websocket
{
	class ClientSet
	{
		Mutex m_mutex;
		Map<Client2*, Unique<Client2>> m_clients;
	public:
		ClientSet(Allocator* allocator)
			: m_mutex(allocator),
			  m_clients(allocator)
		{}

		void push(Unique<Client2> client)
		{
			auto lock = Lock<Mutex>::lock(m_mutex);
			auto handle = client.get();
			m_clients.insert(handle, std::move(client));
		}

		void pop(Client2* client)
		{
			auto lock = Lock<Mutex>::lock(m_mutex);
			m_clients.remove(client);
		}

		void stopAllAndClear()
		{
			auto lock = Lock<Mutex>::lock(m_mutex);
			m_clients.clear();
		}
	};

	class AcceptThread: public EventThread2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		Shared<EventThread2> m_handler;
		ClientSet m_clientSet;
		size_t m_maxHandshakeSize;
	public:
		explicit AcceptThread(EventSocket2 socket, const Shared<EventThread2>& handler, size_t maxHandshakeSize, EventLoop2* eventLoop, Log* log, Allocator* allocator)
			: EventThread2(eventLoop),
			  m_allocator(allocator),
			  m_log(log),
			  m_socket(socket),
			  m_handler(handler),
			  m_clientSet(allocator),
			  m_maxHandshakeSize(maxHandshakeSize)
		{}

		~AcceptThread() override
		{
			m_clientSet.stopAllAndClear();
		}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				return m_socket.accept(sharedFromThis());
			}
			else if (auto acceptEvent = dynamic_cast<AcceptEvent2*>(event))
			{
				auto loop = eventLoop()->next();
				auto socketResult = loop->registerSocket(acceptEvent->releaseSocket());
				if (socketResult.isError())
					return socketResult.releaseError();

				auto clientResult = ImplClient2::accept(socketResult.releaseValue(), m_handler, m_maxHandshakeSize, loop, m_log, m_allocator);
				if (clientResult.isError())
					return clientResult.releaseError();
				m_clientSet.push(clientResult.releaseValue());
				return m_socket.accept(sharedFromThis());
			}
			return {};
		}
	};

	class ImplServer2: public Server2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		Shared<EventThread2> m_acceptThread;
	public:
		ImplServer2(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log)
		{}

		~ImplServer2() override
		{
			(void)m_acceptThread->stop();
		}

		HumanError start(const ServerConfig2& config, EventLoop2* loop) override
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

			loop->startThread<AcceptThread>(eventSocket, config.handler, config.maxHandshakeSize, loop, m_log, m_allocator);
			return {};
		}
	};

	Result<Unique<Server2>> Server2::create(Log *log, Allocator *allocator)
	{
		return unique_from<ImplServer2>(allocator, log, allocator);
	}
}
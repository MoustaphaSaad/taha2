#include "core/websocket/Server2.h"

namespace core::websocket
{
	class HandshakeThread: public EventThread2
	{
		EventSocket2 m_socket;
	public:
		HandshakeThread(EventSocket2 socket, EventLoop2* loop)
			: EventThread2(loop),
			  m_socket(socket)
		{}

		HumanError handle(Event2* event) override
		{
			return {};
		}
	};

	class AcceptThread: public EventThread2
	{
		EventSocket2 m_socket;
		Shared<EventThread2> m_handler;
		Array<Shared<EventThread2>> m_threads;
		size_t m_maxHandshakeSize;
	public:
		explicit AcceptThread(EventSocket2 socket, Shared<EventThread2> handler, size_t maxHandshakeSize, EventLoop2* eventLoop, Allocator* allocator)
			: EventThread2(eventLoop),
			  m_socket(socket),
			  m_handler(handler),
			  m_threads(allocator),
			  m_maxHandshakeSize(maxHandshakeSize)
		{}

		~AcceptThread() override
		{
			for (auto thread: m_threads)
				thread->stop();
		}

		HumanError handle(Event2* event) override
		{
			if (auto startEvent = dynamic_cast<StartEvent2*>(event))
			{
				return m_socket.accept(sharedFromThis());
			}
			else if (auto acceptEvent = dynamic_cast<AcceptEvent2*>(event))
			{
				auto loop = eventLoop();
				auto socketResult = loop->registerSocket(acceptEvent->releaseSocket());
				if (socketResult.isError())
					return socketResult.releaseError();

				auto handshake = loop->startThread<HandshakeThread>(socketResult.releaseValue(), loop);
				m_threads.push(handshake);
				return m_socket.accept(sharedFromThis());
			}
			return {};
		}
	};

	HumanError Server2::start(const ServerConfig2 &config, EventLoop2 *loop)
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

		m_acceptThread = loop->startThread<AcceptThread>(eventSocket, config.handler, config.maxHandshakeSize, loop, m_allocator);
		return {};
	}
}
#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/EventLoop.h>
#include <core/Socket.h>
#include <core/Hash.h>

#include <signal.h>

core::EventLoop* EVENT_LOOP = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		EVENT_LOOP->stop();
	}
}

class EchoServer;

class EchoClient: public core::Reactor
{
	core::Log* m_log = nullptr;
	core::Unique<core::EventSource> m_eventSource;
	EchoServer* m_server = nullptr;
public:
	explicit EchoClient(EchoServer* acceptor, core::Unique<core::EventSource> eventSource, core::Log* log)
		: m_log(log),
		  m_eventSource(std::move(eventSource)),
		  m_server(acceptor)
	{
		(void)m_eventSource->eventLoop()->read(m_eventSource.get(), this);
	}

	void onRead(const core::ReadEvent* event) override;

	void onWrite(const core::WriteEvent* event) override
	{
		m_log->debug("write: \"{}\""_sv, event->writtenSize());
	}
};

class EchoServer: public core::Reactor
{
	core::Allocator* m_allocator = nullptr;
	core::Log* m_log = nullptr;
	core::Map<EchoClient*, core::Unique<EchoClient>> m_clients;
public:
	explicit EchoServer(core::Log* log, core::Allocator* allocator)
		: m_allocator(allocator),
		  m_log(log),
		  m_clients(allocator)
	{}

	void clientDisconnected(EchoClient* client)
	{
		m_log->debug("disconnected"_sv);
		m_clients.remove(client);
	}

	void onAccept(core::AcceptEvent* event) override
	{
		// create printer here and do stuff with it
		auto socket = event->releaseSocket();

		m_log->debug("accepted socket: {}"_sv, socket->fd());

		auto socketSource = event->eventLoop()->createEventSource(std::move(socket));
		auto client = unique_from<EchoClient>(m_allocator, this, std::move(socketSource), m_log);
		m_clients.insert(client.get(), std::move(client));

		// reschedule accept
		(void)event->eventLoop()->accept(event->source(), this);
	}
};

void EchoClient::onRead(const core::ReadEvent* event)
{
	if (event->buffer().count() == 0)
	{
		m_server->clientDisconnected(this);
	}
	else
	{
		m_log->debug("read: \"{}\""_sv, core::StringView{event->buffer()});
		(void) event->eventLoop()->write(event->source(), this, event->buffer());
		(void) event->eventLoop()->read(event->source(), this);
	}
}

int main()
{
	signal(SIGINT, signalHandler);

	core::Mallocator mallocator{};
	core::Log log{&mallocator};

	auto eventLoopResult = core::EventLoop::create(&log, &mallocator);
	if (eventLoopResult.isError())
	{
		log.critical("failed to create event loop, {}"_sv, eventLoopResult.error());
		return EXIT_FAILURE;
	}
	auto eventLoop = eventLoopResult.releaseValue();
	EVENT_LOOP = eventLoop.get();

	EchoServer echoServer{&log, &mallocator};

	auto acceptSocket = core::Socket::open(&mallocator, core::Socket::FAMILY_IPV4, core::Socket::TYPE_TCP);
	acceptSocket->bind("8080"_sv);
	acceptSocket->listen();
	auto acceptEventSocket = eventLoop->createEventSource(std::move(acceptSocket));
	auto err = eventLoop->accept(acceptEventSocket.get(), &echoServer);
	if (err)
	{
		log.critical("socket accept error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	err = eventLoop->run();
	if (err)
	{
		log.critical("event loop error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	return 0;
}
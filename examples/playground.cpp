#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/EventLoop.h>
#include <core/Socket.h>

#include <signal.h>

core::EventLoop* EVENT_LOOP = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		EVENT_LOOP->stop();
	}
}

class Printer: public core::Reactor
{
	core::Log* m_log = nullptr;
public:

	explicit Printer(core::Log* log)
		: m_log(log)
	{}

	void onRead(const core::ReadEvent* event) override
	{
		m_log->debug("read: \"{}\""_sv, core::StringView{event->buffer()});
		event->eventLoop()->read(event->source(), this);
	}
};

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

	Printer printer{&log};

	auto acceptSocket = core::Socket::open(&mallocator, core::Socket::FAMILY_IPV4, core::Socket::TYPE_TCP);
	acceptSocket->bind("8080"_sv);
	acceptSocket->listen();
	auto socket = acceptSocket->accept();

	auto eventSocket = eventLoop->createEventSource(std::move(socket));
	auto err = eventLoop->read(eventSocket.get(), &printer);
	if (err)
	{
		log.critical("socket read error, {}"_sv, err);
	}

	err = eventLoop->run();
	if (err)
	{
		log.critical("event loop error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	return 0;
}
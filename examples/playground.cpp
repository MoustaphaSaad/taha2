#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/websocket/Server2.h>
#include <signal.h>

core::EventLoop* EVENT_LOOP = nullptr;

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		EVENT_LOOP->stop();
	}
}

core::HumanError onMsg(const core::websocket::Message& msg, core::websocket::Server2* server, core::websocket::Conn* conn)
{
	// TODO: handle message
	return {};
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

	auto serverResult = core::websocket::Server2::create(&log, &mallocator);
	if (serverResult.isError())
	{
		log.critical("failed to create websocket server, {}"_sv, eventLoopResult.error());
		return EXIT_FAILURE;
	}
	auto server = serverResult.releaseValue();

	core::websocket::ServerConfig2 config{};
	core::websocket::ServerHandler2 handler{};
	handler.onMsg = onMsg;
	auto err = server->start(config, eventLoop.get(), &handler);
	if (err)
	{
		log.critical("failed to start websocket server, {}"_sv, err);
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
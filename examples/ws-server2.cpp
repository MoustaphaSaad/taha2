#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/ThreadedEventLoop2.h>
#include <core/websocket/Server2.h>
#include <core/Url.h>

#include <signal.h>

core::ThreadedEventLoop2* EVENT_LOOP = nullptr;
void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		EVENT_LOOP->stop();
	}
}

class ServerHandler: public core::EventThread2
{
	core::websocket::Server2* m_server = nullptr;
	core::Log* m_log = nullptr;
public:
	ServerHandler(core::EventLoop2* eventLoop, core::websocket::Server2* server, core::Log* log)
		: EventThread2(eventLoop),
		  m_server(server),
		  m_log(log)
	{}

	core::HumanError handle(core::Event2* event) override
	{
		if (auto newConn = dynamic_cast<core::websocket::NewConnection*>(event))
		{
			return m_server->handleClient(newConn->releaseSocket(), sharedFromThis());
		}
		return {};
	}
};

int main(int argc, char** argv)
{
	auto url = "ws://localhost:9011"_sv;
	if (argc > 1)
		url = core::StringView{argv[1]};

	signal(SIGINT, signalHandler);

	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto parsedUrlResult = core::Url::parse(url, &allocator);
	if (parsedUrlResult.isError())
	{
		log.error("parsing url failed, {}"_sv, parsedUrlResult.error());
		return EXIT_FAILURE;
	}
	auto parsedUrl = parsedUrlResult.releaseValue();

	auto threadedEventLoopResult = core::ThreadedEventLoop2::create(&log, &allocator);
	if (threadedEventLoopResult.isError())
	{
		log.critical("failed to create threaded event loop, {}"_sv, threadedEventLoopResult.releaseError());
		return EXIT_FAILURE;
	}
	auto threadedEventLoop = threadedEventLoopResult.releaseValue();
	EVENT_LOOP = threadedEventLoop.get();

	auto server = core::websocket::Server2{&log, &allocator};

	auto eventLoop = threadedEventLoop->next();
	auto serverHandler = eventLoop->startThread<ServerHandler>(eventLoop, &server, &log);

	core::websocket::ServerConfig2 config {
		.host = parsedUrl.host(),
		.port = parsedUrl.port(),
		.maxHandshakeSize = 64ULL * 1024ULL * 1024ULL,
		.handler = serverHandler,
	};
	auto err = server.start(config, eventLoop->next());
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

	log.debug("success"_sv);
	return EXIT_SUCCESS;
}
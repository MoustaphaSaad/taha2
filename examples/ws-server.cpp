#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/EventLoop.h>
#include <core/websocket/Server.h>
#include <core/websocket/Client.h>
#include <core/Url.h>

#include <tracy/Tracy.hpp>

#include <signal.h>

core::ThreadedEventLoop* EVENT_LOOP = nullptr;
void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		EVENT_LOOP->stop();
	}
}

class ClientHandler: public core::EventThread
{
public:
	ClientHandler(core::EventLoop* eventLoop)
		: EventThread(eventLoop)
	{}

	core::HumanError handle(core::Event* event) override
	{
		if (auto messageEvent = dynamic_cast<core::websocket::MessageEvent*>(event))
		{
			ZoneScopedN("MessageEvent");
			if (messageEvent->message().type == core::websocket::Message::TYPE_TEXT)
			{
				return messageEvent->client()->writeText(core::StringView{messageEvent->message().payload});
			}
			else if (messageEvent->message().type == core::websocket::Message::TYPE_BINARY)
			{
				return messageEvent->client()->writeBinary(core::Span<const std::byte>{messageEvent->message().payload});
			}
			else
			{
				return messageEvent->handle();
			}
		}
		else if (auto errorEvent = dynamic_cast<core::websocket::ErrorEvent*>(event))
		{
			ZoneScopedN("ErrorEvent");
			return errorEvent->handle();
		}
		return {};
	}
};

class ServerHandler: public core::EventThread
{
	core::Log* m_log = nullptr;
public:
	ServerHandler(core::EventLoop* eventLoop, core::Log* log)
		: EventThread(eventLoop),
		  m_log(log)
	{}

	core::HumanError handle(core::Event* event) override
	{
		if (auto newConn = dynamic_cast<core::websocket::NewConnection*>(event))
		{
			ZoneScopedN("NewConnection");
			auto loop = eventLoop()->next();
			auto clientHandler = loop->startThread<ClientHandler>(loop);
			return newConn->client()->startReadingMessages(clientHandler);
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

	auto threadedEventLoopResult = core::ThreadedEventLoop::create(&log, &allocator);
	if (threadedEventLoopResult.isError())
	{
		log.critical("failed to create threaded event loop, {}"_sv, threadedEventLoopResult.releaseError());
		return EXIT_FAILURE;
	}
	auto threadedEventLoop = threadedEventLoopResult.releaseValue();
	EVENT_LOOP = threadedEventLoop.get();

	auto server = core::websocket::Server::create(&log, &allocator);

	auto eventLoop = threadedEventLoop->next();
	auto serverHandler = eventLoop->startThread<ServerHandler>(eventLoop, &log);

	core::websocket::ServerConfig config {
		.host = parsedUrl.host(),
		.port = parsedUrl.port(),
		.maxHandshakeSize = 64ULL * 1024ULL * 1024ULL,
		.handler = serverHandler,
	};
	auto err = server->start(config, eventLoop->next());
	if (err)
	{
		log.critical("failed to start websocket server, {}"_sv, err);
		return EXIT_FAILURE;
	}

	err = threadedEventLoop->run();
	if (err)
	{
		log.critical("event loop error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	log.debug("success"_sv);
	return EXIT_SUCCESS;
}
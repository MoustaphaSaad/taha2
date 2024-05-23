// command to run the wstest
// docker run -it --rm
// -v "W:\Projects\taha2\test\autobahn-testsuite\config:/config"
// -v "W:\Projects\taha2\test\autobahn-testsuite\reports:/reports"
// -p 9011:9011 --name wstest crossbario/autobahn-testsuite wstest -m fuzzingclient -s /config/fuzzingclient.json

#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/ws/Server.h>
#include <core/Url.h>
#include <core/Thread.h>

#include <tracy/Tracy.hpp>

#include <signal.h>

bool RUNNING = true;

void signalHandler(int signal)
{
	if (signal == SIGINT)
		RUNNING = false;
}

core::HumanError handleClient(core::ws::Client client)
{
	while (RUNNING)
	{
		auto messageResult = client.readMessage();
		if (messageResult.isError())
			return messageResult.releaseError();
		auto message = messageResult.releaseValue();

		if (message.kind == core::ws::Message::KIND_TEXT)
		{
			if (auto err = client.writeText(core::StringView{message.payload}))
				return err;
		}
		else if (message.kind == core::ws::Message::KIND_BINARY)
		{
			if (auto err = client.writeBinary(core::Span<const std::byte>{message.payload}))
				return err;
		}
		else if (message.kind == core::ws::Message::KIND_CLOSE)
		{
			return client.handleMessage(message);
		}
		else
		{
			if (auto err = client.handleMessage(message))
				return err;
		}
	}
	return {};
}

int main(int argc, char** argv)
{
	auto url = "ws://localhost:9011"_sv;
	if (argc > 1)
		url = core::StringView{argv[1]};

	signal(SIGINT, signalHandler);

	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto serverResult = core::ws::Server::connect(url, 4096ULL, 64ULL * 1024ULL * 1024ULL, &log, &allocator);
	if (serverResult.isError())
	{
		log.critical("failed to create server, {}"_sv, serverResult.releaseError());
		return EXIT_FAILURE;
	}
	auto server = serverResult.releaseValue();

	while (RUNNING)
	{
		auto clientResult = server.accept();
		if (clientResult.isError())
		{
			log.error("error in accepting a new client, {}"_sv, clientResult.releaseError());
			break;
		}

		core::Func<void()> func{&allocator, [client = clientResult.releaseValue()]() mutable {
			(void)handleClient(std::move(client));
		}};
		core::Thread clientThread{&allocator, std::move(func)};
		clientThread.detach();
	}

	log.debug("success"_sv);
	return EXIT_SUCCESS;
}
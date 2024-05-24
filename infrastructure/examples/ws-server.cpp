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
#include <core/Lock.h>
#include <core/WaitGroup.h>

#include <tracy/Tracy.hpp>

#include <signal.h>

class EchoServer
{
	struct ClientData
	{
		template<typename TPtr, typename... TArgs>
		friend inline core::Shared<TPtr> core::shared_from(core::Allocator* allocator, TArgs&&... args);

		core::ws::Client client;
		core::Unique<core::Thread> thread;

		explicit ClientData(core::ws::Client client_)
			: client(std::move(client_))
		{}
	};

	core::Allocator* m_allocator = nullptr;
	core::ws::Server m_server;
	core::Mutex m_mutex;
	core::Set<core::Shared<ClientData>> m_clients;
	core::WaitGroup m_waitgroup;

	void pushClient(const core::Shared<ClientData>& clientData)
	{
		auto lock = core::lockGuard(m_mutex);
		m_clients.insert(clientData);
	}

	void popClient(const core::Shared<ClientData>& clientData)
	{
		auto lock = core::lockGuard(m_mutex);
		m_clients.remove(clientData);
	}

	static core::HumanError handleClient(const core::Shared<ClientData>& clientData)
	{
		auto& client = clientData->client;
		while (true)
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
	}

	EchoServer(core::ws::Server server, core::Allocator* allocator)
		: m_allocator(allocator),
		  m_server(std::move(server)),
		  m_mutex(allocator),
		  m_clients(allocator),
		  m_waitgroup(allocator)
	{}
public:
	static core::Result<EchoServer> create(core::StringView url, core::Log* log, core::Allocator* allocator)
	{
		auto serverResult = core::ws::Server::connect(url, 4096ULL, 64ULL * 1024ULL * 1024ULL, log, allocator);
		if (serverResult.isError())
			return errf(allocator, "failed to create server, {}"_sv, serverResult.releaseError());
		return EchoServer{serverResult.releaseValue(), allocator};
	}

	void run()
	{
		while (true)
		{
			auto clientResult = m_server.accept();
			if (clientResult.isError())
				break;
			auto client = clientResult.releaseValue();
			auto clientData = core::shared_from<ClientData>(m_allocator, std::move(client));
			m_waitgroup.add(1);
			core::Thread clientThread{m_allocator, [this, clientData]{
				pushClient(clientData);
				(void)handleClient(clientData);
				popClient(clientData);
				m_waitgroup.done();
			}};
			clientData->thread = core::unique_from<core::Thread>(m_allocator, std::move(clientThread));
		}

		{
			auto lock = core::lockGuard(m_mutex);
			for (const auto& clientData: m_clients)
				clientData->client.close();
		}
		m_waitgroup.wait();
	}

	void close()
	{
		m_server.close();
	}
};

EchoServer* SERVER;

void signalHandler(int signal)
{
	if (signal == SIGINT)
		SERVER->close();
}

int main(int argc, char** argv)
{
	auto url = "ws://localhost:9011"_sv;
	if (argc > 1)
		url = core::StringView{argv[1]};

	signal(SIGINT, signalHandler);

	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto serverResult = EchoServer::create(url, &log, &allocator);
	if (serverResult.isError())
	{
		log.critical("failed to create echo server instance, {}"_sv, serverResult.error());
		return EXIT_FAILURE;
	}
	auto server = serverResult.releaseValue();
	SERVER = &server;

	server.run();

	log.debug("success"_sv);
	return EXIT_SUCCESS;
}
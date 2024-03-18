#pragma once

#include "core/Exports.h"
#include "core/websocket/Client.h"
#include "core/StringView.h"
#include "core/EventLoop.h"
#include "core/Unique.h"
#include "core/Hash.h"
#include "core/Mutex.h"

namespace core::websocket
{
	class AcceptThread;

	struct ServerConfig
	{
		StringView host = "127.0.0.1"_sv;
		StringView port = "8080"_sv;
		size_t maxHandshakeSize = 1ULL * 1024ULL;
		size_t maxMessageSize = 64ULL * 1024ULL * 1024ULL;
		Shared<EventThread> handler;
	};

	class NewConnection: public Event
	{
		Shared<Client> m_client;
	public:
		NewConnection(const Shared<Client>& client)
			: m_client(client)
		{}

		const Shared<Client>& client() const { return m_client; }
	};

	class Server
	{
		friend class Client;
		friend class AcceptThread;

		template<typename T, typename... TArgs>
		friend inline Unique<T>
		core::unique_from(Allocator* allocator, TArgs&&... args);

		class ClientSet
		{
			Mutex m_mutex;
			Set<Shared<Client>> m_clients;
		public:
			ClientSet(Allocator* allocator);
			void push(const Shared<Client>& client);
			void pop(const Shared<Client>& client);
		};

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		size_t m_maxHandshakeSize = 0;
		size_t m_maxMessageSize = 0;
		Shared<EventThread> m_acceptThread;
		Shared<EventThread> m_handler;
		ClientSet m_clientSet;

		void clientConnected(Unique<Socket> socket);
		void clientHandshakeDone(const Shared<Client>& client);
		void clientClosed(const Shared<Client>& client);
		Server(Log* log, Allocator* allocator);
	public:
		CORE_EXPORT static Unique<Server> create(Log* log, Allocator* allocator);

		CORE_EXPORT HumanError start(const ServerConfig& config, EventLoop* loop);
	};
}
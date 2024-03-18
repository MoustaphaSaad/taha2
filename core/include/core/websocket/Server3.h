#pragma once

#include "core/Exports.h"
#include "core/websocket/Client3.h"
#include "core/StringView.h"
#include "core/EventLoop.h"
#include "core/Unique.h"
#include "core/Hash.h"
#include "core/Mutex.h"

namespace core::websocket
{
	class AcceptThread3;

	struct ServerConfig3
	{
		StringView host = "127.0.0.1"_sv;
		StringView port = "8080"_sv;
		size_t maxHandshakeSize = 1ULL * 1024ULL;
		size_t maxMessageSize = 64ULL * 1024ULL * 1024ULL;
		Shared<EventThread2> handler;
	};

	class NewConnection3: public Event2
	{
		Shared<Client3> m_client;
	public:
		NewConnection3(const Shared<Client3>& client)
			: m_client(client)
		{}

		const Shared<Client3>& client() const { return m_client; }
	};

	class Server3
	{
		friend class Client3;
		friend class AcceptThread3;

		template<typename T, typename... TArgs>
		friend inline Unique<T>
		core::unique_from(Allocator* allocator, TArgs&&... args);

		class ClientSet
		{
			Mutex m_mutex;
			Set<Shared<Client3>> m_clients;
		public:
			ClientSet(Allocator* allocator);
			void push(const Shared<Client3>& client);
			void pop(const Shared<Client3>& client);
		};

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		size_t m_maxHandshakeSize = 0;
		size_t m_maxMessageSize = 0;
		Shared<EventThread2> m_acceptThread;
		Shared<EventThread2> m_handler;
		ClientSet m_clientSet;

		void clientConnected(Unique<Socket> socket);
		void clientHandshakeDone(const Shared<Client3>& client);
		void clientClosed(const Shared<Client3>& client);
		Server3(Log* log, Allocator* allocator);
	public:
		CORE_EXPORT static Unique<Server3> create(Log* log, Allocator* allocator);

		CORE_EXPORT HumanError start(const ServerConfig3& config, EventLoop2* loop);
	};
}
#pragma once

#include "core/Exports.h"
#include "core/EventLoop2.h"
#include "core/Shared.h"

namespace core::websocket
{
	class Server3;
	class ServerHandshakeThread3;
	class ReadMessageThread3;

	class Client3: public SharedFromThis<Client3>
	{
		friend class Server3;
		friend class ServerHandshakeThread3;
		friend class ReadMessageThread3;

		template<typename T, typename ... TArgs>
		friend inline Shared<T>
		core::shared_from(Allocator* allocator, TArgs&& ... args);

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket2 m_socket;
		size_t m_maxMessageSize = 64ULL * 1024ULL * 1024ULL;
		Server3* m_server = nullptr;

		void handshakeDone(bool success);
		void connectionClosed();
		Client3(EventSocket2 socket, size_t m_maxMessageSize, Log* log, Allocator* allocator);
		static Shared<Client3> acceptFromServer(
			Server3* server,
			EventLoop2* loop,
			EventSocket2 socket,
			size_t maxHandshakeSize,
			size_t maxMessageSize,
			Log* log,
			Allocator* allocator
		);
	public:
		CORE_EXPORT HumanError startReadingMessages(const Shared<EventThread2>& handler);
	};
}
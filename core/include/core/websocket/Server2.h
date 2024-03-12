#pragma once

#include "core/Exports.h"
#include "core/EventLoop2.h"
#include "core/ThreadedEventLoop2.h"
#include "core/websocket/Message.h"
#include "core/websocket/Client2.h"

namespace core::websocket
{
	class NewConnection: public Event2
	{
		Client2* m_client = nullptr;
	public:
		NewConnection(Client2* client)
			: m_client(client)
		{}

		Client2* client() const { return m_client; }
	};

	class MessageEvent: public Event2
	{
		Message m_message;
	public:
		MessageEvent(Message&& message)
			: m_message(std::move(message))
		{}

		Message& message() { return m_message; }
		const Message& message() const { return m_message; }
	};

	struct ServerConfig2
	{
		StringView host = "127.0.0.1"_sv;
		StringView port = "8080"_sv;
		size_t maxHandshakeSize = 1ULL * 1024ULL;
		Shared<EventThread2> handler;
	};

	class Server2
	{
	public:
		CORE_EXPORT static Result<Unique<Server2>> create(Log* log, Allocator* allocator);

		virtual ~Server2() = default;
		virtual HumanError start(const ServerConfig2& config, EventLoop2* loop) = 0;
	};
}
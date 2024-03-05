#pragma once

#include "core/Exports.h"
#include "core/EventLoop2.h"
#include "core/ThreadedEventLoop2.h"
#include "core/websocket/Message.h"

namespace core::websocket
{
	class NewConnection: public Event2
	{
		EventSocket2 m_socket;
	public:
		NewConnection(EventSocket2 socket)
			: m_socket(socket)
		{}

		EventSocket2 releaseSocket() { return std::move(m_socket); }
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
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		Shared<EventThread2> m_acceptThread;
	public:
		Server2(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log)
		{}

		CORE_EXPORT HumanError start(const ServerConfig2& config, EventLoop2* loop);
		CORE_EXPORT HumanError handleClient(EventSocket2 socket, const Shared<EventThread2>& handler);
	};
}
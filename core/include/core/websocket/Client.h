#pragma once

#include "core/Exports.h"
#include "core/websocket/Message.h"
#include "core/websocket/MessageParser.h"
#include "core/EventLoop.h"
#include "core/Shared.h"

namespace core::websocket
{
	class Server;
	class ServerHandshakeThread;
	class ClientHandshakeThread;
	class ReadMessageThread;

	class Client: public SharedFromThis<Client>
	{
		friend class Server;
		friend class ServerHandshakeThread;
		friend class ClientHandshakeThread;
		friend class ReadMessageThread;

		template<typename T, typename ... TArgs>
		friend inline Shared<T>
		core::shared_from(Allocator* allocator, TArgs&& ... args);

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		EventSocket m_socket;
		size_t m_maxMessageSize = 64ULL * 1024ULL * 1024ULL;
		Server* m_server = nullptr;
		Buffer m_buffer;
		Shared<EventThread> m_handler;

		HumanError writeFrame(FrameHeader::OPCODE opcode, Span<const std::byte> payload);
		HumanError writeCloseWithCode(uint16_t code, StringView reason);
		void handshakeDone(bool success);
		void connectionClosed();
		Client(EventSocket socket, size_t m_maxMessageSize, Log* log, Allocator* allocator);
		static Shared<Client> acceptFromServer(
				Server* server,
				EventLoop* loop,
				EventSocket socket,
				size_t maxHandshakeSize,
				size_t maxMessageSize,
				Log* log,
				Allocator* allocator
		);
	public:
		CORE_EXPORT static Result<Shared<Client>> connect(StringView url, size_t maxHandshakeSize, size_t maxMessageSize, const Shared<EventThread>& handler, Log* log, Allocator* allocator);
		CORE_EXPORT HumanError startReadingMessages(const Shared<EventThread>& handler);

		CORE_EXPORT HumanError writeText(StringView payload);
		CORE_EXPORT HumanError writeBinary(Span<const std::byte> payload);
		CORE_EXPORT HumanError writePing(Span<const std::byte> payload);
		CORE_EXPORT HumanError writePong(Span<const std::byte> payload);
		CORE_EXPORT HumanError writeClose(uint16_t errorCode, StringView optionalReason = ""_sv);
		CORE_EXPORT HumanError defaultMessageHandler(const Message& message);
	};

	class MessageEvent: public Event
	{
		Shared<Client> m_client;
		Message m_message;
	public:
		MessageEvent(const Shared<Client>& client, Message message)
			: m_client(client),
			  m_message(std::move(message))
		{}

		const Shared<Client>& client() const { return m_client; }
		const Message& message() const { return m_message; }

		HumanError handle()
		{
			return m_client->defaultMessageHandler(m_message);
		}
	};

	class ErrorEvent: public Event
	{
		Shared<Client> m_client;
		HumanError m_error;
		uint16_t m_errorCode = 1002;
	public:
		ErrorEvent(const Shared<Client>& client, uint16_t errorCode, HumanError error)
			: m_client(client),
			  m_error(std::move(error)),
			  m_errorCode(errorCode)
		{}

		const Shared<Client>& client() const { return m_client; }
		const HumanError& error() const { return m_error; }
		uint16_t errorCode() const { return m_errorCode; }

		HumanError handle()
		{
			return m_client->writeClose(m_errorCode, m_error.message());
		}
	};

	class DisconnectedEvent: public Event
	{
		Shared<Client> m_client;
	public:
		DisconnectedEvent(const Shared<Client>& client)
			: m_client(client)
		{}

		const Shared<Client>& client() const { return m_client; }
	};
}
#pragma once

#include "core/Exports.h"
#include "core/websocket/Message.h"
#include "core/websocket/MessageParser.h"
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

		HumanError writeFrame(FrameHeader::OPCODE opcode, Span<const std::byte> payload);
		HumanError writeCloseWithCode(uint16_t code, StringView reason);
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

		CORE_EXPORT HumanError writeText(StringView payload);
		CORE_EXPORT HumanError writeBinary(Span<const std::byte> payload);
		CORE_EXPORT HumanError writePing(Span<const std::byte> payload);
		CORE_EXPORT HumanError writePong(Span<const std::byte> payload);
		CORE_EXPORT HumanError writeClose(uint16_t errorCode, StringView optionalReason = ""_sv);
		CORE_EXPORT HumanError defaultMessageHandler(const Message& message);
	};

	class MessageEvent: public Event2
	{
		Shared<Client3> m_client;
		Message m_message;
	public:
		MessageEvent(const Shared<Client3>& client, Message message)
			: m_client(client),
			  m_message(std::move(message))
		{}

		const Shared<Client3>& client() const { return m_client; }
		const Message& message() const { return m_message; }

		HumanError handle()
		{
			return m_client->defaultMessageHandler(m_message);
		}
	};

	class ErrorEvent: public Event2
	{
		Shared<Client3> m_client;
		HumanError m_error;
		uint16_t m_errorCode = 1002;
	public:
		ErrorEvent(const Shared<Client3>& client, uint16_t errorCode, HumanError error)
			: m_client(client),
			  m_error(std::move(error)),
			  m_errorCode(errorCode)
		{}

		const Shared<Client3>& client() const { return m_client; }
		const HumanError& error() const { return m_error; }
		uint16_t errorCode() const { return m_errorCode; }

		HumanError handle()
		{
			return m_client->writeClose(m_errorCode, m_error.message());
		}
	};
}
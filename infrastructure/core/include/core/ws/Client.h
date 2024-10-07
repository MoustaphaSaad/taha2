#pragma once

#include "core/BufferedReader.h"
#include "core/Exports.h"
#include "core/Mutex.h"
#include "core/Result.h"
#include "core/Socket.h"
#include "core/Url.h"
#include "core/ws/Message.h"

namespace core::ws
{
	class Client
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		Unique<Socket> m_socket;
		BufferedReader m_bufferedReader;
		MessageParser m_messageParser;
		Mutex m_writeMutex;
		size_t m_maxHandshakeSize = 0;
		size_t m_maxMessageSize = 0;
		bool m_shouldMask = false;

		HumanError write(Span<const std::byte> bytes);
		HumanError read(Span<std::byte> bytes);
		HumanError sendHandshake(const Url& url, const String& base64Key);
		Result<String> readHTTP(size_t maxSize);
		HumanError handshake(const Url& url);
		HumanError serverHandshake();
		HumanError writeFrame(Frame::OPCODE opcode, Span<const std::byte> payload);
		HumanError writeCloseWithCode(uint16_t code, StringView reason);

		Client(Unique<Socket> socket, size_t maxHandshakeSize, size_t maxMessageSize, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_socket(std::move(socket)),
			  m_bufferedReader(m_socket.get(), allocator),
			  m_messageParser(maxMessageSize, allocator),
			  m_writeMutex(allocator),
			  m_maxHandshakeSize(maxHandshakeSize),
			  m_maxMessageSize(maxMessageSize)
		{}

	public:
		CORE_EXPORT static Result<Client>
		connect(StringView url, size_t maxHandshakeSize, size_t maxMessageSize, Log* log, Allocator* allocator);
		CORE_EXPORT static Result<Client> acceptFromServer(
			Unique<Socket> socket, size_t maxHandshakeSize, size_t maxMessageSize, Log* log, Allocator* allocator);

		CORE_EXPORT Result<Message> readMessage();
		CORE_EXPORT HumanError handleMessage(const Message& message);
		CORE_EXPORT HumanError writeText(StringView payload);
		CORE_EXPORT HumanError writeBinary(Span<const std::byte> payload);
		CORE_EXPORT HumanError writePing(Span<const std::byte> payload);
		CORE_EXPORT HumanError writePong(Span<const std::byte> payload);
		CORE_EXPORT HumanError writeClose(uint16_t code, StringView reason);

		CORE_EXPORT void close();
	};
}
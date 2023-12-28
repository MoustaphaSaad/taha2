#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Result.h"
#include "core/Log.h"
#include "core/Socket.h"
#include "core/Func.h"
#include "core/websocket/Message.h"
#include "core/websocket/MessageParser.h"

namespace core::websocket
{
	class Client;

	struct ClientHandler
	{
		bool handlePing = false;
		bool handlePong = false;
		bool handleClose = false;
		size_t maxMessageSize = 16ULL * 1024ULL * 1024ULL;

		Func<HumanError(const Message&, Client*)> onMsg;
	};

	class Client
	{
		Allocator* m_allocator = nullptr;
		Log* m_logger = nullptr;
		Unique<Socket> m_socket;
		std::byte m_key[16] = {};
		std::byte m_recvLine[1024] = {};
		Buffer m_recvBuffer;
		MessageParser m_messageParser;

		HumanError handshake(StringView path);
		HumanError readAndHandleMessage();
	public:
		static CORE_EXPORT Result<Unique<Client>> connect(StringView ip, StringView port, Log* log, Allocator* allocator);

		Client(Unique<Socket> socket, Log* logger, Allocator* allocator)
			: m_allocator(allocator),
			  m_logger(logger),
			  m_socket(std::move(socket)),
			  m_recvBuffer(allocator),
			  m_messageParser(allocator, 16ULL * 1024ULL * 1024ULL)
		{}

		HumanError run();
	};
}
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

	struct ClientConfig
	{
		StringView ip = "127.0.0.1"_sv;
		StringView port = "8080"_sv;
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
		ClientConfig m_config;

		HumanError handshake(StringView path);
		HumanError readAndHandleMessage();
		HumanError onMsg(const Message& msg);

	public:
		static CORE_EXPORT Result<Unique<Client>> connect(ClientConfig&& config, Log* log, Allocator* allocator);

		Client(ClientConfig&& config, Unique<Socket> socket, Log* logger, Allocator* allocator)
			: m_allocator(allocator),
			  m_logger(logger),
			  m_socket(std::move(socket)),
			  m_recvBuffer(allocator),
			  m_messageParser(allocator, 16ULL * 1024ULL * 1024ULL),
			  m_config(std::move(config))
		{}

		HumanError run();
	};
}
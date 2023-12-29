#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Result.h"
#include "core/Log.h"
#include "core/Socket.h"
#include "core/Func.h"
#include "core/Url.h"
#include "core/websocket/Message.h"
#include "core/websocket/MessageParser.h"

namespace core::websocket
{
	class Client;

	struct ClientConfig
	{
		StringView url = "ws://127.0.0.1:8080"_sv;
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
		Url m_url;

		HumanError handshake();
		HumanError readAndHandleMessage();
		HumanError onMsg(const Message& msg);
		HumanError writeRaw(Span<const std::byte> bytes);
		HumanError writeFrameHeader(FrameHeader::OPCODE opcode, size_t payloadLength);
		HumanError writeFrame(FrameHeader::OPCODE opcode, Span<const std::byte> payload);

	public:
		static CORE_EXPORT Result<Unique<Client>> connect(ClientConfig&& config, Log* log, Allocator* allocator);

		Client(ClientConfig&& config, Unique<Socket> socket, Url&& url, Log* logger, Allocator* allocator)
			: m_allocator(allocator),
			  m_logger(logger),
			  m_socket(std::move(socket)),
			  m_recvBuffer(allocator),
			  m_messageParser(allocator, 16ULL * 1024ULL * 1024ULL),
			  m_config(std::move(config)),
			  m_url(std::move(url))
		{
		}

		CORE_EXPORT HumanError run();
		CORE_EXPORT void stop();

		CORE_EXPORT HumanError writeText(StringView str);
		CORE_EXPORT HumanError writeBinary(Span<const std::byte> bytes);
		CORE_EXPORT HumanError writePing(Span<const std::byte> bytes);
		CORE_EXPORT HumanError writePong(Span<const std::byte> bytes);
		CORE_EXPORT HumanError writeClose();
		CORE_EXPORT HumanError writeClose(uint16_t code);
	};
}
#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Result.h"
#include "core/Log.h"
#include "core/Allocator.h"
#include "core/EventLoop.h"
#include "core/Func.h"
#include "core/websocket/Message.h"

namespace core::websocket
{
	class Client;

	struct ClientConfig
	{
		bool handlePing = false;
		bool handlePong = false;
		bool handleClose = false;
		size_t maxMessageSize = 16ULL * 1024ULL * 1024ULL;
		Func<HumanError(const Message&, Client*)> onMsg;
		Func<HumanError(Client*)> onConnected;
		Func<HumanError()> onDisconnected;
	};

	class Client
	{
	public:
		static CORE_EXPORT Result<Unique<Client>> connect(StringView url, ClientConfig&& config, EventLoop* loop, Log* log, Allocator* allocator);

		virtual ~Client() = default;

		virtual HumanError start() = 0;
		virtual void stop() = 0;

		virtual HumanError writeText(StringView str) = 0;
		virtual HumanError writeBinary(Span<const std::byte> bytes) = 0;
		virtual HumanError writePing(Span<const std::byte> bytes) = 0;
		virtual HumanError writePong(Span<const std::byte> bytes) = 0;
		virtual HumanError writeClose() = 0;
		virtual HumanError writeClose(uint16_t code, StringView optionalReason) = 0;
	};
}
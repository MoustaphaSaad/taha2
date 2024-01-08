#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Result.h"
#include "core/Log.h"
#include "core/Func.h"
#include "core/StringView.h"
#include "core/EventLoop.h"
#include "core/websocket/Message.h"

namespace core::websocket
{
	class XXConn;
	class XXServer;

	class XXServer
	{
	public:
		static CORE_EXPORT Result<Unique<XXServer>> open(Log* log, Allocator* allocator);

		virtual ~XXServer();
		virtual HumanError start(EventLoop* loop, StringView ip, StringView port) = 0;
		// virtual void stop() = 0;
		// virtual HumanError writeText(Connection* conn, StringView str) = 0;
		// virtual HumanError writeText(Connection* conn, Buffer&& str) = 0;
		// virtual HumanError writeBinary(Connection* conn, Span<const std::byte> bytes) = 0;
		// virtual HumanError writeBinary(Connection* conn, Buffer&& bytes) = 0;
		// virtual HumanError writePing(Connection* conn, Span<const std::byte> bytes) = 0;
		// virtual HumanError writePing(Connection* conn, Buffer&& bytes) = 0;
		// virtual HumanError writePong(Connection* conn, Span<const std::byte> bytes) = 0;
		// virtual HumanError writePong(Connection* conn, Buffer&& bytes) = 0;
		// virtual HumanError writeClose(Connection* conn) = 0;
		// virtual HumanError writeClose(Connection* conn, uint16_t code) = 0;
	};
}
#pragma once

#include "core/Exports.h"
#include "core/Unique.h"
#include "core/Result.h"
#include "core/Log.h"
#include "core/Socket.h"

namespace core::websocket
{
	class Client
	{
		Allocator* m_allocator = nullptr;
		Log* m_logger = nullptr;
		Unique<Socket> m_socket;
		std::byte m_key[16] = {};

		HumanError handshake(StringView path);
	public:
		static CORE_EXPORT Result<Unique<Client>> connect(StringView ip, StringView port, Log* log, Allocator* allocator);

		Client(Unique<Socket> socket, Log* logger, Allocator* allocator)
			: m_allocator(allocator),
			  m_logger(logger),
			  m_socket(std::move(socket))
		{}

		HumanError run();
	};
}
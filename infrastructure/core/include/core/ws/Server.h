#pragma once

#include "core/Exports.h"
#include "core/Socket.h"
#include "core/Log.h"
#include "core/Result.h"
#include "core/ws/Client.h"

namespace core::ws
{
	class Server
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		Unique<Socket> m_socket;
		size_t m_maxHandshakeSize = 0;
		size_t m_maxMessageSize = 0;

		Server(Unique<Socket> socket, size_t maxHandshakeSize, size_t maxMessageSize, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_socket(std::move(socket)),
			  m_maxHandshakeSize(maxHandshakeSize),
			  m_maxMessageSize(maxMessageSize)
		{}
	public:
		CORE_EXPORT static Result<Server> connect(StringView url, size_t maxHandshakeSize, size_t maxMessageSize, Log* log, Allocator* allocator);

		CORE_EXPORT Result<Client> accept();
		CORE_EXPORT void close();
	};
}
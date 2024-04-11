#pragma once

#include "fin/Exports.h"

#include <core/Result.h>
#include <core/Unique.h>
#include <core/StringView.h>
#include <core/Log.h>
#include <core/Allocator.h>
#include <core/EventLoop.h>
#include <core/websocket/Server.h>
#include <core/IPCMutex.h>
#include <core/Lock.h>

namespace fin
{
	class LockInfo
	{
		core::String m_absPath;
		core::String m_lockName;
		core::String m_portFilePath;

		LockInfo(core::String absPath, core::String lockName, core::String filePath)
			: m_absPath(std::move(absPath)),
			  m_lockName(std::move(lockName)),
			  m_portFilePath(std::move(filePath))
		{}

	public:
		FIN_EXPORT static core::Result<LockInfo> create(core::StringView path, core::Allocator* allocator);

		core::StringView absPath() const { return m_absPath; }
		core::StringView lockName() const { return m_lockName; }
		core::StringView portFilePath() const { return m_portFilePath; }
	};

	class Server
	{
		template<typename T, typename... TArgs>
		friend inline core::Unique<T>
		core::unique_from(core::Allocator* allocator, TArgs&&... args);

		core::EventLoop* m_eventLoop = nullptr;
		core::Unique<core::websocket::Server> m_server;
		core::Shared<core::EventThread> m_serverThread;
		LockInfo m_lockInfo;
		core::IPCMutex m_ipcMutex;
		core::Lock<core::IPCMutex> m_ipcMutexLock;

		Server(
			core::EventLoop* eventLoop,
			core::Unique<core::websocket::Server> server,
			core::Shared<core::EventThread> serverThread,
			LockInfo lockInfo,
			core::IPCMutex ipcMutex,
			core::Lock<core::IPCMutex> ipcMutexLock
		)
			: m_eventLoop(std::move(eventLoop)),
			  m_server(std::move(server)),
			  m_serverThread(std::move(serverThread)),
			  m_lockInfo(std::move(lockInfo)),
			  m_ipcMutex(std::move(ipcMutex)),
			  m_ipcMutexLock(std::move(ipcMutexLock))
		{}

	public:
		enum class FLAG
		{
			NONE          = 0 << 0,
			CREATE_LEDGER = 1 << 0,
		};

		FIN_EXPORT static core::Result<core::Unique<Server>> create(
			core::StringView path,
			core::StringView url,
			FLAG flags,
			core::EventLoop* eventLoop,
			core::Log* log,
			core::Allocator* allocator
		);

		uint16_t listeningPort() const { return m_server->listeningPort(); }
	};

	inline Server::FLAG operator|(Server::FLAG f1, Server::FLAG f2)
	{
		return Server::FLAG(int(f1) | int(f2));
	}

	inline Server::FLAG& operator|=(Server::FLAG& f1, Server::FLAG f2)
	{
		f1 = f1 | f2;
		return f1;
	}
}
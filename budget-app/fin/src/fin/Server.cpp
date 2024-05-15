#include "fin/Server.h"

#include <core/Path.h>
#include <core/SHA1.h>
#include <core/MemoryStream.h>
#include <core/Url.h>
#include <core/File.h>

namespace fin
{
	class ClientHandler: public core::EventThread
	{
	public:
		ClientHandler(core::EventLoop* eventLoop)
			: EventThread(eventLoop)
		{}

		core::HumanError handle(core::Event* event) override
		{
			if (auto messageEvent = dynamic_cast<core::websocket::MessageEvent*>(event))
			{
				if (messageEvent->message().type == core::websocket::Message::TYPE_TEXT)
				{
					return messageEvent->client()->writeText(core::StringView{messageEvent->message().payload});
				}
				else if (messageEvent->message().type == core::websocket::Message::TYPE_BINARY)
				{
					return messageEvent->client()->writeBinary(core::Span<const std::byte>{messageEvent->message().payload});
				}
				else
				{
					return messageEvent->handle();
				}
			}
			else if (auto errorEvent = dynamic_cast<core::websocket::ErrorEvent*>(event))
			{
				return errorEvent->handle();
			}
			return {};
		}
	};

	class ServerHandler: public core::EventThread
	{
		core::Log* m_log = nullptr;
	public:
		ServerHandler(core::EventLoop* eventLoop, core::Log* log)
			: EventThread(eventLoop),
			  m_log(log)
		{}

		core::HumanError handle(core::Event* event) override
		{
			if (auto newConn = dynamic_cast<core::websocket::NewConnection*>(event))
			{
				auto loop = eventLoop()->next();
				auto clientHandler = loop->startThread<ClientHandler>(loop);
				return newConn->client()->startReadingMessages(clientHandler);
			}
			return {};
		}
	};

	core::Result<LockInfo> LockInfo::create(core::StringView path, core::Allocator* allocator)
	{
		auto absolutePathResult = core::Path::abs(path, allocator);
		if (absolutePathResult.isError())
			return absolutePathResult.releaseError();
		auto absolutePath = absolutePathResult.releaseValue();

		auto hash = core::SHA1::hash(absolutePath);
		auto tmpResult = core::Path::tmpDir(allocator);
		if (tmpResult.isError())
			return tmpResult.releaseError();

		core::MemoryStream stream{allocator};
		for (auto b: hash.asBytes())
			core::strf(&stream, "{:02x}"_sv, b);
		auto lockName = stream.releaseString();

		core::strf(&stream, "budget_byte_{}"_sv, lockName);
		auto filePath = core::Path::join(allocator, tmpResult.releaseValue(), stream.releaseString());

		return LockInfo{std::move(absolutePath), std::move(lockName), std::move(filePath)};
	}

	core::Result<core::Unique<Server>> Server::create(
		core::StringView path,
		core::StringView url,
		FLAG flags,
		core::EventLoop* eventLoop,
		core::Log *log,
		core::Allocator *allocator
	)
	{
		auto lockInfoResult = LockInfo::create(path, allocator);
		if (lockInfoResult.isError())
			return lockInfoResult.releaseError();
		auto lockInfo = lockInfoResult.releaseValue();

		log->debug("{}"_sv, lockInfo.portFilePath());

		auto ipcMutex = core::IPCMutex{lockInfo.lockName(), allocator};
		auto lock = core::tryLockGuard(ipcMutex);
		if (lock.isLocked() == false)
			return core::errf(allocator, "failed to acquire lock on file '{}'"_sv, lockInfo.absPath());

		auto parsedUrlResult = core::Url::parse(url, allocator);
		if (parsedUrlResult.isError())
			return parsedUrlResult.releaseError();
		auto parsedUrl = parsedUrlResult.releaseValue();

		auto server = core::websocket::Server::create(log, allocator);

		auto serverHandler = eventLoop->startThread<ServerHandler>(eventLoop, log);

		core::websocket::ServerConfig config
		{
			.host = parsedUrl.host().count() > 0 ? parsedUrl.host() : "localhost"_sv,
			.port = parsedUrl.port().count() > 0 ? parsedUrl.port() : "0"_sv,
			.maxHandshakeSize = 64ULL * 1024ULL * 1024ULL,
			.handler = serverHandler,
		};
		auto err = server->start(config, eventLoop);
		if (err)
			return err;

		auto file = core::File::open(allocator, lockInfo.portFilePath(), core::File::IO_MODE_WRITE, core::File::OPEN_MODE_CREATE_OVERWRITE, core::File::SHARE_MODE_NONE);
		if (file == nullptr)
			return core::errf(allocator, "failed to write listening port to port file '{}'"_sv, lockInfo.portFilePath());
		core::strf(file.get(), "{}"_sv, server->listeningPort());

		return core::unique_from<Server>(allocator,
			std::move(eventLoop),
			std::move(server),
			std::move(serverHandler),
			std::move(lockInfo),
			std::move(ipcMutex),
			std::move(lock)
		);
	}
}
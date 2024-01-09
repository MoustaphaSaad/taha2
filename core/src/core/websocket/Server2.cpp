#include "core/websocket/Server2.h"

namespace core::websocket
{
	class Server2Impl: public Server2
	{
		class AcceptHandler: public Reactor
		{
			Log* m_log = nullptr;
			Unique<EventSource> m_socketSource;
		public:
			static Result<Unique<AcceptHandler>> create(StringView host, StringView port, EventLoop* loop, Log* log, Allocator* allocator)
			{
				auto acceptSocket = Socket::open(allocator, Socket::FAMILY_IPV4, Socket::TYPE_TCP);
				if (acceptSocket == nullptr)
					return errf(allocator, "failed to open accept socket"_sv);

				auto ok = acceptSocket->bind(host, port);
				if (ok == false)
					return errf(allocator, "failed to bind accept socket to {}:{}"_sv, host, port);

				ok = acceptSocket->listen();
				if (ok == false)
					return errf(allocator, "failed to listen to {}:{}"_sv, host, port);

				auto socketSource = loop->createEventSource(std::move(acceptSocket));
				if (socketSource == nullptr)
					return errf(allocator, "failed to convert socket to event loop source"_sv);

				auto res = unique_from<AcceptHandler>(allocator, std::move(socketSource), log);
				auto err = loop->accept(res->m_socketSource.get(), res.get());
				if (err)
					return err;

				return res;
			}

			explicit AcceptHandler(Unique<EventSource> socketSource, Log* log)
				: m_socketSource(std::move(socketSource)),
				  m_log(log)
			{}

			void onAccept(AcceptEvent* event) override
			{
				// TODO: handle new connections
				m_log->debug("new connection"_sv);
			}
		};

		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		Unique<AcceptHandler> m_acceptHandler;
	public:
		Server2Impl(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log)
		{}

		HumanError start(ServerConfig2 config, EventLoop* loop) override
		{
			auto acceptHandlerResult = AcceptHandler::create(config.host, config.port, loop, m_log, m_allocator);
			if (acceptHandlerResult.isError())
				return acceptHandlerResult.releaseError();
			m_acceptHandler = acceptHandlerResult.releaseValue();
			return {};
		}
	};

	Result<Unique<Server2>> Server2::create(Log *log, Allocator *allocator)
	{
		return unique_from<Server2Impl>(allocator, log, allocator);
	}
}
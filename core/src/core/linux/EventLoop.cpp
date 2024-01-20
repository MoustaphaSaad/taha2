#include "core/EventLoop.h"

#include <sys/epoll.h>

namespace core
{
	class LinuxEventLoop: public EventLoop
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		int m_epoll = 0;
	public:
		LinuxEventLoop(int epoll, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_epoll(epoll)
		{}

		~LinuxEventLoop() override
		{
			if (m_epoll)
			{
				auto res = close(m_epoll);
				assert(res == 0);
			}
		}

		HumanError run() override
		{
			// TODO: implement this function
			return {};
		}

		void stop() override
		{
			// TODO: implement this function
		}

		Shared<EventSource> createEventSource(Unique<Socket> socket) override
		{
			// TODO: implement this function
			return nullptr;
		}

		HumanError read(EventSource* source, Reactor* reactor) override
		{
			// TODO: implement this function
			return {};
		}

		HumanError write(EventSource* source, Reactor* reactor, Span<const std::byte> buffer) override
		{
			// TODO: implement this function
			return {};
		}

		HumanError accept(EventSource* source, Reactor* reactor) override
		{
			// TODO: implement this function
			return {};
		}
	};

	Result<Unique<EventLoop>> EventLoop::create(Log* log, Allocator* allocatr)
	{
		int epoll_fd = epoll_create1(0);
		if (epoll_fd == -1)
			return errf(allocatr, "failed to create epoll"_sv);

		return unique_from<LinuxEventLoop>(allocatr, epoll_fd, log, allocatr);
	}
}
#include "core/EventLoop.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>

namespace core
{
	class LinuxEventLoop: public EventLoop
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		int m_epoll = 0;
		int m_closeEvent = 0;
	public:
		LinuxEventLoop(int epoll, int closeEvent, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_epoll(epoll),
			  m_closeEvent(closeEvent)
		{}

		~LinuxEventLoop() override
		{
			if (m_epoll)
			{
				[[maybe_unused]] auto res = close(m_epoll);
				assert(res == 0);
			}

			if (m_closeEvent)
			{
				[[maybe_unused]] auto res = close(m_closeEvent);
				assert(res == 0);
			}
		}

		HumanError run() override
		{
			constexpr int MAX_EVENTS = 32;
			epoll_event events[MAX_EVENTS];
			while (true)
			{
				auto count = epoll_wait(m_epoll, events, MAX_EVENTS, -1);
				for (size_t i = 0; i < count; ++i)
				{
					// TODO: handle events
				}
			}

			return {};
		}

		void stop() override
		{
			int64_t close = 1;
			[[maybe_unused]] auto res = ::write(m_closeEvent, &close, sizeof(close));
			assert(res == sizeof(close));
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

	Result<Unique<EventLoop>> EventLoop::create(Log* log, Allocator* allocator)
	{
		int epoll_fd = epoll_create1(0);
		if (epoll_fd == -1)
			return errf(allocator, "failed to create epoll"_sv);

		int closeEvent = eventfd(0, 0);
		if (closeEvent == -1)
			return errf(allocator, "failed to create close event"_sv);

		epoll_event signal{};
		signal.events = EPOLLIN;
		signal.data.fd = closeEvent;
		int ok = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, closeEvent, &signal);
		if (ok == -1)
			return errf(allocator, "failed to add close event to epoll instance"_sv);

		return unique_from<LinuxEventLoop>(allocator, epoll_fd, closeEvent, log, allocator);
	}
}
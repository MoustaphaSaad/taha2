#include "core/EventThread.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <tracy/Tracy.hpp>

namespace core
{
	class LinuxThreadPool: public EventThreadPool
	{
		Log* m_log = nullptr;
		int m_epoll = -1;
		int m_closefd = -1;
		ThreadPool* m_threadPool = nullptr;

		class LinuxEventSocket: public EventSource2
		{
			LinuxThreadPool* m_eventThreadPool = nullptr;
			Unique<Socket> m_socket;
		public:
			LinuxEventSocket(Unique<Socket> socket, LinuxThreadPool* eventThreadPool)
				: m_eventThreadPool(eventThreadPool),
				  m_socket(std::move(socket))
			{}

			HumanError accept(const Shared<EventThread>& thread) override
			{
				return errf(m_eventThreadPool->m_allocator, "not implemented"_sv);
			}

			HumanError read(const Shared<EventThread>& thread) override
			{
				return errf(m_eventThreadPool->m_allocator, "not implemented"_sv);
			}

			HumanError write(Span<const std::byte> buffer, const Shared<EventThread>& thread) override
			{
				return errf(m_eventThreadPool->m_allocator, "not implemented"_sv);
			}
		};

		void addThread(const Shared<EventThread>& thread) override
		{
			assert(false && "NOT IMPLEMENTED");
		}
	protected:
		HumanError sendEvent(Unique<Event2> event, const Shared<EventThread>& thread) override
		{
			assert(false && "NOT IMPLEMENTED");
			return {};
		}

	public:
		LinuxThreadPool(ThreadPool* threadPool, int epoll, Log* log, Allocator* allocator)
			: EventThreadPool(allocator),
			  m_log(log),
			  m_epoll(epoll),
			  m_threadPool(threadPool)
		{}

		~LinuxThreadPool() override
		{
			if (m_closefd != -1)
			{
				[[maybe_unused]] auto res = ::close(m_closefd);
				assert(res == 0);
				m_closefd = -1;
			}

			if (m_epoll == -1)
			{
				[[maybe_unused]] auto res = ::close(m_epoll);
				assert(res == 0);
				m_epoll = -1;
			}
		}

		HumanError run() override
		{
			m_closefd = eventfd(0, 0);
			if (m_closefd == -1)
				return errf(m_allocator, "failed to create close event, ErrorCode({})"_sv, errno);

			epoll_event closeEvent{};
			closeEvent.events = EPOLLIN | EPOLLONESHOT;
			closeEvent.data.fd = m_closefd;
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_closefd, &closeEvent);
			if (ok == -1)
				return errf(m_allocator, "failed to add close event to epoll, ErrorCode({})"_sv, errno);

			constexpr int MAX_EVENTS = 128;
			epoll_event events[MAX_EVENTS];
			while (true)
			{
				int numEntries = 0;
				{
					ZoneScopedN("EventThreadPool::epoll_wait");
					numEntries = epoll_wait(m_epoll, events, MAX_EVENTS, -1);
					if (numEntries == -1)
					{
						if (errno == EINTR)
						{
							// try again, this can be due to signal handling, like debugger
							continue;
						}
						return errf(m_allocator, "epoll_wait failed, ErrorCode({})"_sv, errno);
					}
				}

				for (int i = 0; i < numEntries; ++i)
				{
					auto event = events[i];
					if (event.data.fd == m_closefd)
					{
						m_log->debug("event loop closed"_sv);
						[[maybe_unused]] auto res = ::close(m_closefd);
						assert(res == 0);
						m_closefd = -1;
						return {};
					}
				}
			}
			return {};
		}

		void stop() override
		{
			int64_t close = 1;
			[[maybe_unused]] auto res = ::write(m_closefd, &close, sizeof(close));
			assert(res == sizeof(close));
		}

		Result<EventSocket> registerSocket(Unique<Socket> socket) override
		{
			// TODO: add socket to epoll instance
			auto res = shared_from<LinuxEventSocket>(m_allocator, std::move(socket), this);
			return EventSocket{res};
		}

		void stopThread(const Shared<EventThread>& thread) override
		{
			assert(false && "NOT IMPLEMENTED");
		}
	};

	Result<Unique<EventThreadPool>> EventThreadPool::create(ThreadPool *threadPool, Log *log, Allocator *allocator)
	{
		auto epoll = epoll_create1(0);
		if (epoll == -1)
			return errf(allocator, "failed to create epoll, ErrorCode({})"_sv, errno);

		return unique_from<LinuxThreadPool>(allocator, threadPool, epoll, log, allocator);
	}
}
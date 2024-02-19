#include "core/EventThread.h"

#include <sys/epoll.h>

namespace core
{
	class LinuxThreadPool: public EventThreadPool
	{
		Log* m_log = nullptr;
		int m_epoll = -1;
		ThreadPool* m_threadPool = nullptr;

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

		HumanError run() override
		{
			assert(false && "NOT IMPLEMENTED");
			return {};
		}

		void stop() override
		{
			assert(false && "NOT IMPLEMENTED");
		}

		HumanError registerSocket(const Unique<Socket>& socket) override
		{
			assert(false && "NOT IMPLEMENTED");
			return {};
		}

		HumanError accept(const Unique<Socket>& socket, const Shared<EventThread>& thread) override
		{
			assert(false && "NOT IMPLEMENTED");
			return {};
		}

		HumanError read(const Unique<Socket>& socket, const Shared<EventThread>& thread) override
		{
			assert(false && "NOT IMPLEMENTED");
			return {};
		}

		HumanError write(const Unique<Socket>& socket, const Shared<EventThread>& thread, Span<const std::byte> buffer) override
		{
			assert(false && "NOT IMPLEMENTED");
			return {};
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
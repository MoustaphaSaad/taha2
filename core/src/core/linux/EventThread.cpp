#include "core/EventThread.h"
#include "core/Hash.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <tracy/Tracy.hpp>

namespace core
{
	class LinuxThreadPool: public EventThreadPool
	{
		struct Op
		{
			enum KIND
			{
				KIND_NONE,
				KIND_CLOSE,
			};

			explicit Op(KIND kind_)
				: kind(kind_)
			{}

			KIND kind;
		};

		struct CloseOp: Op
		{
			CloseOp()
				: Op(KIND_CLOSE)
			{}
		};

		class LinuxEventSource: public EventSource2
		{
			Allocator* m_allocator = nullptr;
			Queue<Unique<Op>> m_pollInOps;
			Queue<Unique<Op>> m_pollOutOps;
		public:
			explicit LinuxEventSource(Allocator* allocator)
				: m_allocator(allocator),
				  m_pollInOps(allocator),
				  m_pollOutOps(allocator)
			{}

			void pushPollIn(Unique<Op> op)
			{
				assert(op != nullptr);
				m_pollInOps.push_back(std::move(op));
			}

			Unique<Op> popPollIn()
			{
				if (m_pollInOps.count() > 0)
				{
					auto res = std::move(m_pollInOps.front());
					m_pollInOps.pop_front();
					return res;
				}
				return nullptr;
			}

			void pushPollOut(Unique<Op> op)
			{
				assert(op != nullptr);
				m_pollOutOps.push_back(std::move(op));
			}

			Unique<Op> popPollOut()
			{
				if (m_pollOutOps.count() > 0)
				{
					auto res = std::move(m_pollOutOps.front());
					m_pollOutOps.pop_front();
					return res;
				}
				return nullptr;
			}

			HumanError accept(const Shared<EventThread>& thread) override
			{
				return errf(m_allocator, "not implemented"_sv);
			}

			HumanError read(const Shared<EventThread>& thread) override
			{
				return errf(m_allocator, "not implemented"_sv);
			}

			HumanError write(Span<const std::byte> buffer, const Shared<EventThread>& thread) override
			{
				return errf(m_allocator, "not implemented"_sv);
			}
		};

		class LinuxCloseEventSource: public LinuxEventSource
		{
			int m_eventfd = -1;
		public:
			LinuxCloseEventSource(int eventfd, Allocator* allocator)
				: LinuxEventSource(allocator),
				  m_eventfd(eventfd)
			{}

			~LinuxCloseEventSource() override
			{
				if (m_eventfd != -1)
				{
					[[maybe_unused]] auto res = ::close(m_eventfd);
					assert(res == 0);
				}
			}

			void signalClose()
			{
				ZoneScoped;
				int64_t close = 1;
				[[maybe_unused]] auto res = ::write(m_eventfd, &close, sizeof(close));
				assert(res == sizeof(close));
			}
		};

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

		class SourceSet
		{
			Mutex m_mutex;
			Map<LinuxEventSource*, Weak<LinuxEventSource>> m_sources;
		public:
			explicit SourceSet(Allocator* allocator)
				: m_mutex(allocator),
				  m_sources(allocator)
			{}

			void pushSource(const Shared<LinuxEventSource>& source)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				auto key = source.get();
				m_sources.insert(key, source);
			}

			void removeSource(LinuxEventSource* source)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_sources.remove(source);
			}

			Shared<LinuxEventSource> popSource(LinuxEventSource* source)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				auto it = m_sources.lookup(source);
				if (it == m_sources.end())
					return nullptr;
				return it->value.lock();
			}
		};

		Log* m_log = nullptr;
		int m_epoll = -1;
		Shared<LinuxCloseEventSource> m_closeEventSource;
		SourceSet m_sources;
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
			  m_sources(allocator),
			  m_threadPool(threadPool)
		{}

		~LinuxThreadPool() override
		{
			if (m_epoll == -1)
			{
				[[maybe_unused]] auto res = ::close(m_epoll);
				assert(res == 0);
				m_epoll = -1;
			}
		}

		HumanError run() override
		{
			auto closefd = eventfd(0, 0);
			if (closefd == -1)
				return errf(m_allocator, "failed to create close event, ErrorCode({})"_sv, errno);
			m_closeEventSource = shared_from<LinuxCloseEventSource>(m_allocator, closefd, m_allocator);
			m_sources.pushSource(m_closeEventSource);
			m_closeEventSource->pushPollIn(unique_from<CloseOp>(m_allocator));

			epoll_event closeEvent{};
			closeEvent.events = EPOLLIN | EPOLLONESHOT;
			closeEvent.data.ptr = m_closeEventSource.get();
			auto ok = epoll_ctl(m_epoll, EPOLL_CTL_ADD, closefd, &closeEvent);
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
					auto sourceHandle = (LinuxEventSource*)event.data.ptr;
					auto source = m_sources.popSource(sourceHandle);
					assert(source != nullptr);

					if (event.events | EPOLLIN)
					{
						// close event
						if (source.get() == (LinuxEventSource*)m_closeEventSource.get())
						{
							m_log->debug("event loop closed"_sv);
							m_closeEventSource = nullptr;
							return {};
						}
					}
				}
			}
			return {};
		}

		void stop() override
		{
			ZoneScoped;
			m_log->debug("stop"_sv);
			m_closeEventSource->signalClose();
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
#include "core/EventThread.h"
#include "core/Hash.h"
#include "core/Mutex.h"

#include <Windows.h>

namespace core
{
	class WinOSThreadPool: public EventThreadPool
	{
		struct Op: OVERLAPPED
		{
			enum KIND
			{
				KIND_NONE,
				KIND_CLOSE,
				KIND_SEND_EVENT,
			};

			explicit Op(KIND kind_)
				: kind(kind_)
			{}

			virtual ~Op() = default;

			KIND kind = KIND_NONE;
		};

		struct CloseOp: Op
		{
			CloseOp()
				: Op(KIND_CLOSE)
			{}
		};

		struct SendEventOp: Op
		{
			SendEventOp(Unique<Event2> event_, EventThread* thread_)
				: Op(KIND_SEND_EVENT),
				  event(std::move(event_)),
				  thread(thread_)
			{}

			Unique<Event2> event;
			EventThread* thread = nullptr;
		};

		class OpSet
		{
			Mutex m_mutex;
			Map<OVERLAPPED*, Unique<Op>> m_ops;
			bool m_open = true;
		public:
			explicit OpSet(Allocator* allocator)
				: m_ops(allocator),
				  m_mutex(allocator)
			{}

			bool tryPush(Unique<Op> op)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);

				if (m_open == false)
					return false;

				auto handle = (OVERLAPPED*)op.get();
				m_ops.insert(handle, std::move(op));
				return true;
			}

			Unique<Op> pop(OVERLAPPED* op)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);

				auto it = m_ops.lookup(op);
				if (it == m_ops.end())
					return nullptr;
				auto res = std::move(it->value);
				m_ops.remove(op);
				return res;
			}

			void close()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_open = false;
			}

			void open()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_open = true;
			}

			void clear()
			{
				auto lock = Lock<Mutex>::lock(m_mutex);
				m_ops.clear();
			}
		};

		Log* m_log = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
		OpSet m_ops;
		Map<EventThread*, Unique<EventThread>> m_threads;

		void addThread(Unique<EventThread> thread) override
		{
			auto handle = thread.get();
			m_threads.insert(handle, std::move(thread));

			auto startEvent = unique_from<StartEvent>(m_allocator);
			[[maybe_unused]] auto err = sendEvent(std::move(startEvent), handle);
			assert(!err);
		}

	protected:
		HumanError sendEvent(Unique<Event2> event, EventThread* thread) override
		{
			auto op = unique_from<SendEventOp>(m_allocator, std::move(event), thread);
			auto overlapped = (LPOVERLAPPED)op.get();
			if (m_ops.tryPush(std::move(op)))
			{
				auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, overlapped);
				if (FAILED(res))
					return errf(m_allocator, "failed to send event to thread"_sv);
			}
			return {};
		}

	public:
		WinOSThreadPool(HANDLE completionPort, Log* log, Allocator* allocator)
			: EventThreadPool(allocator),
			  m_log(log),
			  m_completionPort(completionPort),
			  m_ops(allocator),
			  m_threads(allocator)
		{}

		HumanError run() override
		{
			m_ops.open();

			while (true)
			{
				constexpr int MAX_ENTRIES = 128;
				OVERLAPPED_ENTRY entries[MAX_ENTRIES];
				ULONG numEntries = 0;
				{
					auto res = GetQueuedCompletionStatusEx(m_completionPort, entries, MAX_ENTRIES, &numEntries, INFINITE, false);
					if (res == FALSE)
					{
						auto error = GetLastError();
						return errf(m_allocator, "Failed to get queued completion status: ErrorCode({})"_sv, error);
					}
				}

				for (size_t i = 0; i < numEntries; ++i)
				{
					auto overlapped = entries[i].lpOverlapped;

					auto op = m_ops.pop(overlapped);
					assert(op != nullptr);

					switch (op->kind)
					{
					case Op::KIND_CLOSE:
					{
						m_ops.clear();
						return {};
					}
					case Op::KIND_SEND_EVENT:
					{
						auto sendEventOp = unique_static_cast<SendEventOp>(std::move(op));
						sendEventOp->thread->handle(sendEventOp->event.get());
						break;
					}
					default:
					{
						assert(false);
						return errf(m_allocator, "unknown internal op kind, {}"_sv, (int)op->kind);
					}
					}
				}
			}
		}

		void stop() override
		{
			auto op = unique_from<CloseOp>(m_allocator);
			auto overlapped = (LPOVERLAPPED)op.get();
			if (m_ops.tryPush(std::move(op)))
			{
				[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, overlapped);
				assert(SUCCEEDED(res));
			}
			m_ops.close();
		}
	};

	Result<Unique<EventThreadPool>> EventThreadPool::create(Log *log, Allocator *allocator)
	{
		auto completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (completionPort == NULL)
			return errf(allocator, "failed to create completion port"_sv);

		auto res = unique_from<WinOSThreadPool>(allocator, completionPort, log, allocator);
		return res;
	}
}
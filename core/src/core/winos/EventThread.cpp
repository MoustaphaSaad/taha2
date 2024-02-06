#include "core/EventThread.h"
#include "core/Hash.h"

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

		Log* m_log = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
		Map<OVERLAPPED*, Unique<Op>> m_scheduledOps;
		Map<EventThread*, Unique<EventThread>> m_threads;

		void pushPendingOp(Unique<Op> op)
		{
			auto handle = (OVERLAPPED*)op.get();
			m_scheduledOps.insert(handle, std::move(op));
		}

		Unique<Op> popPendingOp(OVERLAPPED* op)
		{
			auto it = m_scheduledOps.lookup(op);
			if (it == m_scheduledOps.end())
				return nullptr;
			auto res = std::move(it->value);
			m_scheduledOps.remove(op);
			return res;
		}

		void addThread(Unique<EventThread> thread) override
		{
			auto handle = thread.get();
			m_threads.insert(handle, std::move(thread));

			auto startEvent = unique_from<StartEvent>(m_allocator);
			[[maybe_unused]] auto err = sendEvent(std::move(startEvent), handle);
			assert(!err);
		}

	public:
		WinOSThreadPool(HANDLE completionPort, Log* log, Allocator* allocator)
			: EventThreadPool(allocator),
			  m_log(log),
			  m_completionPort(completionPort),
			  m_scheduledOps(allocator),
			  m_threads(allocator)
		{}

		HumanError run() override
		{
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
					auto overlappedOp = (Op*)overlapped;

					auto op = popPendingOp(overlapped);
					assert(op != nullptr);

					switch (op->kind)
					{
					case Op::KIND_CLOSE:
					{
						m_scheduledOps.clear();
						m_threads.clear();
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
			[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, (LPOVERLAPPED)op.get());
			assert(SUCCEEDED(res));
			pushPendingOp(std::move(op));
		}

		HumanError sendEvent(Unique<Event2> event, EventThread* thread) override
		{
			auto op = unique_from<SendEventOp>(m_allocator, std::move(event), thread);
			auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, (LPOVERLAPPED)op.get());
			if (FAILED(res))
				return errf(m_allocator, "failed to send event to thread"_sv);
			pushPendingOp(std::move(op));
			return {};
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
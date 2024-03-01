#include "core/EventLoop2.h"
#include "core/Mutex.h"
#include "core/Hash.h"

#include <tracy/Tracy.hpp>

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>

namespace core
{
	class WinOSEventLoop2: public EventLoop2
	{
		struct Op: OVERLAPPED
		{
			Op()
			{
				::memset((OVERLAPPED*)this, 0, sizeof(OVERLAPPED));
			}

			virtual ~Op() = default;
		};

		struct CloseOp: Op
		{};

		class OpSet
		{
			Mutex m_mutex;
			Map<Op*, Unique<Op>> m_ops;
			bool m_open = true;
		public:
			explicit OpSet(Allocator *allocator)
					: m_ops(allocator),
					  m_mutex(allocator)
			{}

			bool tryPush(Unique<Op> op)
			{
				auto lock = Lock<Mutex>::lock(m_mutex);

				if (m_open == false)
					return false;

				auto handle = (Op*) op.get();
				m_ops.insert(handle, std::move(op));
				return true;
			}

			Unique<Op> pop(Op* op)
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

		void addThread(const Shared<EventThread2>& thread) override
		{
			assert(false);
		}

		Log* m_log = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
		LPFN_ACCEPTEX m_acceptEx = nullptr;
		OpSet m_ops;
	public:
		WinOSEventLoop2(HANDLE completionPort, LPFN_ACCEPTEX acceptEx, Log* log, Allocator* allocator)
			: EventLoop2(allocator),
			  m_log(log),
			  m_completionPort(completionPort),
			  m_acceptEx(acceptEx),
			  m_ops(allocator)
		{}

		~WinOSEventLoop2() override
		{
			if (m_completionPort != INVALID_HANDLE_VALUE)
			{
				[[maybe_unused]] auto res = CloseHandle(m_completionPort);
				assert(SUCCEEDED(res));
			}
		}

		HumanError run() override
		{
			m_ops.open();

			constexpr int MAX_ENTRIES = 128;
			OVERLAPPED_ENTRY entries[MAX_ENTRIES];
			while (true)
			{
				ULONG numEntries = 0;
				{
					ZoneScopedN("EventLoop2::GetQueuedCompletionStatusEx");
					auto res = GetQueuedCompletionStatusEx(m_completionPort, entries, MAX_ENTRIES, &numEntries, INFINITE, FALSE);
					if (res == FALSE)
					{
						auto error = GetLastError();
						return errf(m_allocator, "Failed to get queued completion status: ErrorCode({})"_sv, error);
					}
				}

				for (size_t i = 0; i < numEntries; ++i)
				{
					auto entry = entries[i];
					if (auto op = m_ops.pop((Op*)entry.lpOverlapped))
					{
						if (auto closeOp = dynamic_cast<CloseOp*>(op.get()))
						{
							m_ops.clear();
							return {};
						}
						else
						{
							assert(false);
							return errf(m_allocator, "unknown op type"_sv);
						}
					}
				}
			}
			return {};
		}

		void stop() override
		{
			auto op = unique_from<CloseOp>(m_allocator);
			auto handle = op.get();
			if (m_ops.tryPush(std::move(op)))
			{
				[[maybe_unused]] auto res = PostQueuedCompletionStatus(m_completionPort, 0, NULL, (OVERLAPPED*)handle);
				assert(SUCCEEDED(res));
			}
		}

		Result<EventSocket2> registerSocket(Unique<Socket> socket) override
		{
			assert(false);
			return errf(m_allocator, "not implemented"_sv);
		}
	};

	Result<Unique<EventLoop2>> EventLoop2::create(Log *log, Allocator *allocator)
	{
		auto completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (completionPort == NULL)
			return errf(allocator, "failed to create completion port"_sv);

		auto dummySocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (dummySocket == INVALID_SOCKET)
			return errf(allocator, "failed to create dummy socket"_sv);

		GUID guidAcceptEx = WSAID_ACCEPTEX;
		LPFN_ACCEPTEX acceptEx = nullptr;
		DWORD bytesReturned = 0;
		auto res = WSAIoctl(
			dummySocket,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidAcceptEx,
			sizeof(guidAcceptEx),
			&acceptEx,
			sizeof(acceptEx),
			&bytesReturned,
			NULL,
			NULL
		);
		if (res == SOCKET_ERROR)
			return errf(allocator, "failed to get AcceptEx function pointer, ErrorCode({})"_sv, res);

		return unique_from<WinOSEventLoop2>(allocator, completionPort, acceptEx, log, allocator);
	}
}
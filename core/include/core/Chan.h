#pragma once

#include "core/Allocator.h"
#include "core/Mutex.h"
#include "core/Queue.h"
#include "core/ConditionVariable.h"
#include "core/Shared.h"
#include "core/Func.h"

#include <atomic>
#include <random>

namespace core
{
	enum class ChanDir
	{
		Send = -1,
		Both = 0,
		Recv = 1,
	};

	enum class ChanErr
	{
		Ok = 0,
		Closed,
		Empty,
	};

	template<typename T, ChanDir dir = ChanDir::Both>
	class Chan
	{
		template<typename TPtr, typename... TArgs>
		friend inline Shared<TPtr>
		shared_from(Allocator* allocator, TArgs&&... args);

		Allocator* m_allocator = nullptr;
		std::atomic<bool> m_closed = false;
		Mutex m_mutex;
		ConditionVariable m_readCond;
		ConditionVariable m_writeCond;
		std::atomic<int> m_readWaiting = 0;
		std::atomic<int> m_writeWaiting = 0;
		Queue<T> m_buffer;
		size_t m_bufferSize = 0;
		T* m_unbufferedSlot = nullptr;
		Mutex m_readMutex;
		Mutex m_writeMutex;

		explicit Chan(Allocator* allocator)
			: m_allocator(allocator),
			  m_mutex(allocator),
			  m_readCond(allocator),
			  m_writeCond(allocator),
			  m_buffer(allocator),
			  m_readMutex(allocator),
			  m_writeMutex(allocator)
		{}

		ChanErr internalSend(T* ptr)
		{
			if (isBuffered())
			{
				auto lockMutex = Lock<Mutex>::lock(m_mutex);
				while (m_buffer.count() == m_bufferSize)
				{
					m_writeWaiting.fetch_add(1);
					m_writeCond.wait(m_mutex);
					m_writeWaiting.fetch_sub(1);
				}

				coreAssert(m_buffer.count() < m_bufferSize);
				if constexpr (std::is_move_constructible_v<T>)
					m_buffer.push_back(std::move(*ptr));
				else
					m_buffer.push_back(*ptr);
				if (m_readWaiting.load() > 0)
					m_readCond.notify_one();
				return ChanErr::Ok;
			}
			else
			{
				auto lockWriteMutex = Lock<Mutex>::lock(m_writeMutex);
				auto lockMutex = Lock<Mutex>::lock(m_mutex);

				if (m_closed.load())
					return ChanErr::Closed;

				m_unbufferedSlot = ptr;
				m_writeWaiting.fetch_add(1);
				if (m_readWaiting.load() > 0)
					m_readCond.notify_one();
				m_writeCond.wait(m_mutex);
				return ChanErr::Ok;
			}
		}

		Result<T, ChanErr> internalRecv()
		{
			if (isBuffered())
			{
				auto lockMutex = Lock<Mutex>::lock(m_mutex);
				while (m_buffer.count() == 0)
				{
					if (m_closed.load())
						return ChanErr::Closed;

					m_readWaiting.fetch_add(1);
					m_readCond.wait(m_mutex);
					m_readWaiting.fetch_sub(1);
				}

				coreAssert(m_buffer.count() > 0);
				auto res = Result<T, ChanErr>::createEmpty();
				if constexpr (std::is_move_constructible_v<T>)
					res = std::move(m_buffer.front());
				else
					res = m_buffer.front();
				m_buffer.pop_front();

				if (m_writeWaiting.load() > 0)
					m_writeCond.notify_one();
				return res;
			}
			else
			{
				auto lockReadMutex = Lock<Mutex>::lock(m_readMutex);
				auto lockMutex = Lock<Mutex>::lock(m_mutex);

				while (m_closed.load() == false && m_writeWaiting.load() == 0)
				{
					m_readWaiting.fetch_add(1);
					m_readCond.wait(m_mutex);
					m_readWaiting.fetch_sub(1);
				}

				if (m_closed.load())
					return ChanErr::Closed;

				auto res = Result<T, ChanErr>::createEmpty();
				if constexpr (std::is_move_constructible_v<T>)
					res = std::move(*m_unbufferedSlot);
				else
					res = *m_unbufferedSlot;
				m_unbufferedSlot = nullptr;
				m_writeWaiting.fetch_sub(1);
				m_writeCond.notify_one();
				return res;
			}
		}

		ChanErr internalTrySend(T* ptr)
		{
			if (isBuffered())
			{
				auto lockMutex = Lock<Mutex>::lock(m_mutex);
				if (m_buffer.count() == m_bufferSize)
					return ChanErr::Empty;

				coreAssert(m_buffer.count() < m_bufferSize);
				if constexpr (std::is_move_constructible_v<T>)
					m_buffer.push_back(std::move(*ptr));
				else
					m_buffer.push_back(*ptr);
				if (m_readWaiting.load() > 0)
					m_readCond.notify_one();
				return ChanErr::Ok;
			}
			else
			{
				auto lockWriteMutex = Lock<Mutex>::lock(m_writeMutex);
				auto lockMutex = Lock<Mutex>::lock(m_mutex);

				if (m_closed.load())
					return ChanErr::Closed;

				m_unbufferedSlot = ptr;
				m_writeWaiting.fetch_add(1);
				if (m_readWaiting.load() > 0)
					m_readCond.notify_one();
				m_writeCond.wait(m_mutex);
				return ChanErr::Ok;
			}
		}

		Result<T, ChanErr> internalTryRecv()
		{
			if (isBuffered())
			{
				auto lockMutex = Lock<Mutex>::lock(m_mutex);
				if (m_buffer.count() == 0)
					return ChanErr::Empty;

				coreAssert(m_buffer.count() > 0);
				auto res = Result<T, ChanErr>::createEmpty();
				if constexpr (std::is_move_constructible_v<T>)
					res = std::move(m_buffer.front());
				else
					res = m_buffer.front();
				m_buffer.pop_front();

				if (m_writeWaiting.load() > 0)
					m_writeCond.notify_one();
				return res;
			}
			else
			{
				auto lockReadMutex = Lock<Mutex>::lock(m_readMutex);
				auto lockMutex = Lock<Mutex>::lock(m_mutex);

				if (m_closed.load())
					return ChanErr::Closed;
				else if (m_writeWaiting.load() == 0)
					return ChanErr::Empty;

				auto res = Result<T, ChanErr>::createEmpty();
				if constexpr (std::is_move_constructible_v<T>)
					res = std::move(*m_unbufferedSlot);
				else
					res = *m_unbufferedSlot;
				m_unbufferedSlot = nullptr;
				m_writeWaiting.fetch_sub(1);
				m_writeCond.notify_one();
				return res;
			}
		}

		bool internalClose()
		{
			auto lockMutex = Lock<Mutex>::lock(m_mutex);
			if (m_closed.load())
				return false;
			m_closed.store(true);
			m_readCond.notify_all();
			m_writeCond.notify_all();
			return true;
		}

	public:
		static Shared<Chan<T, dir>> create(size_t bufferCap, Allocator* allocator)
		{
			auto res = shared_from<Chan<T, dir>>(allocator, allocator);
			res->m_bufferSize = bufferCap;
			return res;
		}

		bool isBuffered() const { return m_bufferSize > 0; }
		size_t capacity() const { return m_bufferSize; }
		Allocator* allocator() const { return m_allocator; }

		size_t count()
		{
			if (isBuffered())
			{
				auto lockMutex = Lock<Mutex>::lock(m_mutex);
				return m_buffer.count();
			}
			return 0;
		}

		bool isClosed()
		{
			auto lockMutex = Lock<Mutex>::lock(m_mutex);
			return m_closed.load();
		}

		ChanErr send(T* value)
		{
			coreAssert(value != nullptr);
			return internalSend(value);
		}

		Result<T, ChanErr> recv()
		{
			return internalRecv();
		}

		ChanErr trySend(T* value)
		{
			coreAssert(value != nullptr);
			return internalTrySend(&value);
		}

		Result<T, ChanErr> tryRecv()
		{
			return internalTryRecv();
		}

		bool close()
		{
			return internalClose();
		}

		class ChanIterator
		{
			Chan<T, dir>* m_chan;
			Result<T, ChanErr> m_recvRes;
		public:
			ChanIterator(Chan<T, dir>& chan, Result<T, ChanErr>&& res)
				: m_chan(&chan),
				  m_recvRes(std::move(res))
			{}

			ChanIterator& operator++()
			{
				m_recvRes = m_chan->recv();
				return *this;
			}

			ChanIterator& operator++(int)
			{
				auto r = *this;
				m_recvRes = m_chan->recv();
				return r;
			}

			bool operator==(const ChanIterator& other) const
			{
				return m_chan == other.m_chan && m_recvRes.isError() == other.m_recvRes.isError();
			}

			bool operator!=(const ChanIterator& other) const
			{
				return !operator==(other);
			}

			T& operator*() { return m_recvRes.value(); }
			const T& operator*() const { return m_recvRes.value(); }
			T* operator->() { return &m_recvRes.value(); }
			const T* operator->() const { return &m_recvRes.value(); }
		};

		ChanIterator begin()
		{
			return ChanIterator{*this, recv()};
		}

		ChanIterator end()
		{
			return ChanIterator{*this, ChanErr::Closed};
		}
	};

	class SelectCaseDesc;

	template<typename T, ChanDir dir>
	class ReadCase
	{
		friend class SelectCaseDesc;
		Allocator* m_allocator = nullptr;
		Chan<T, dir>* m_chan = nullptr;
		Result<T, ChanErr> m_res = Result<T, ChanErr>::createEmpty();
		Func<void(T&&)> m_callback;
	public:
		ReadCase(Allocator* allocator, Chan<T, dir>& chan)
			: m_allocator(allocator),
			  m_chan(&chan)
		{}

		template<typename TFunc>
		ReadCase& operator=(TFunc&& func)
		{
			m_callback = Func<void(T&&)>{m_allocator, std::forward<TFunc>(func)};
			return *this;
		}
	};

	template<typename T, ChanDir dir>
	class WriteCase
	{
		friend class SelectCaseDesc;
		Allocator* m_allocator = nullptr;
		Chan<T, dir>* m_chan = nullptr;
		T* m_value = nullptr;
		Func<void()> m_callback;
	public:
		WriteCase(Allocator* allocator, Chan<T, dir>& chan, T& value)
			: m_allocator(allocator),
			  m_chan(&chan),
			  m_value(&value)
		{}

		WriteCase(Chan<T, dir>& chan, T value)
			: m_chan(&chan)
		{}

		template<typename TFunc>
		WriteCase& operator=(TFunc&& func)
		{
			m_callback = Func<void()>{m_allocator, std::forward<TFunc>(func)};
			return *this;
		}
	};

	class DefaultCase
	{
		friend class SelectCaseDesc;
		Allocator* m_allocator = nullptr;
		Func<void()> m_callback;
	public:
		DefaultCase(Allocator* allocator)
			: m_allocator(allocator)
		{}

		template<typename TFunc>
		DefaultCase& operator=(TFunc&& func)
		{
			m_callback = Func<void()>{m_allocator, std::forward<TFunc>(func)};
			return *this;
		}
	};

	class SelectCaseDesc
	{
		void* m_case = nullptr;
		ChanErr (*m_eval)(void* ptr) = nullptr;
		bool m_isDefault = false;
	public:
		template<typename T, ChanDir dir>
		SelectCaseDesc(ReadCase<T, dir>& c)
			: m_case(&c)
		{
			m_eval = +[](void* ptr) {
				auto c = (ReadCase<T, dir>*)ptr;
				c->m_res = c->m_chan->tryRecv();
				if (c->m_res.isError())
				{
					if (c->m_res.error() == ChanErr::Closed)
						return c->m_res.releaseError();
				}
				else if (c->m_res.isValue())
				{
					c->m_callback(c->m_res.releaseValue());
					return ChanErr::Ok;
				}
				return ChanErr::Empty;
			};
		}

		template<typename T, ChanDir dir>
		SelectCaseDesc(WriteCase<T, dir>& c)
			: m_case(&c)
		{
			m_eval = +[](void* ptr) {
				auto c = (WriteCase<T, dir>*)ptr;
				auto err = c->m_chan->trySend(c->m_value);
				if (err == ChanErr::Ok)
					c->m_callback();
				return err;
			};
		}

		SelectCaseDesc(DefaultCase& c)
			: m_case(&c),
			  m_isDefault(true)
		{
			m_eval = +[](void* ptr) {
				auto c = (DefaultCase*)ptr;
				c->m_callback();
				return ChanErr::Ok;
			};
		}

		bool isDefault() const { return m_isDefault; }
		ChanErr eval()
		{
			return m_eval(m_case);
		}
	};

	class Select
	{
	public:
		template<typename ... TArgs>
		Select(TArgs&& ... cases)
		{
			SelectCaseDesc descs[] = {cases...};
			auto descsCount = sizeof...(cases);
			size_t defaultIndex = descsCount;
			for (size_t i = 0; i < descsCount; ++i)
			{
				if (descs[i].isDefault())
				{
					// TODO: replace it with a panic
					coreAssert(defaultIndex == descsCount);
					defaultIndex = i;
				}
			}
			auto hasDefault = defaultIndex < descsCount;
			auto terminateCount = hasDefault ? 1 : 0;

			std::random_device device;
			std::uniform_int_distribution<size_t> dist(0, descsCount);
			while (descsCount > terminateCount)
			{
				auto offsetIndex = dist(device);
				for (size_t i = 0; i < descsCount; ++i)
				{
					auto index = (i + offsetIndex) % descsCount;
					if (i != defaultIndex)
					{
						auto err = descs[index].eval();
						if (err == ChanErr::Ok)
						{
							if (hasDefault)
								return;
							else
								continue;
						}
						else if (err == ChanErr::Closed)
						{
							// swap and delete
							if (index != descsCount - 1)
							{
								auto tmp = std::move(descs[descsCount - 1]);
								descs[descsCount - 1] = std::move(descs[index]);
								descs[index] = std::move(tmp);
							}
							--descsCount;
						}
					}
				}

				if (hasDefault)
				{
					descs[defaultIndex].eval();
					return;
				}
				else if (descsCount > terminateCount)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
		}
	};
}
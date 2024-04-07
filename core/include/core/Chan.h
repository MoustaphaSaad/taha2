#pragma once

#include "core/Mallocator.h"
#include "core/Allocator.h"
#include "core/Mutex.h"
#include "core/Lock.h"
#include "core/Queue.h"
#include "core/ConditionVariable.h"
#include "core/Shared.h"
#include "core/Func.h"
#include "core/Hash.h"

#include <atomic>
#include <random>

namespace core
{
	struct SelectCond
	{
	public:
		struct Event
		{
			size_t index = SIZE_MAX;
			bool closed = false;

			bool isSignaled() const { return index != SIZE_MAX; }
		};

		explicit SelectCond(Allocator* allocator)
			: m_mutex(allocator),
			  m_waitCond(allocator),
			  m_deliverCond(allocator)
		{}

		bool trySignalReady(size_t index)
		{
			auto lock = lockGuard(m_mutex);
			if (m_event.isSignaled() || m_closed)
				return false;
			m_event = Event{.index = index};
			m_waitCond.notify_one();
			return true;
		}

		bool signalReady(size_t index)
		{
			auto lock = lockGuard(m_mutex);
			while (m_event.isSignaled() && m_closed == false)
				m_deliverCond.wait(m_mutex);
			if (m_closed)
				return false;
			m_event = Event{.index = index};
			m_waitCond.notify_one();
			return true;
		}

		bool signalClose(size_t index)
		{
			auto lock = lockGuard(m_mutex);
			while (m_event.isSignaled() && m_closed == false)
				m_deliverCond.wait(m_mutex);
			if (m_closed)
				return false;
			m_event = Event{.index = index, .closed = true};
			m_waitCond.notify_one();
			return true;
		}

		Event waitForEventAndClose()
		{
			auto lock = lockGuard(m_mutex);
			if (m_event.isSignaled())
			{
				auto res = m_event;
				m_event = Event{};
				return res;
			}

			m_waitCond.wait(m_mutex);
			coreAssert(m_event.isSignaled());
			auto res = m_event;
			m_event = Event{};
			// close if we get real event
			if (res.closed == false)
				m_closed = true;

			m_deliverCond.notify_all();

			return res;
		}

		void close()
		{
			auto lock = lockGuard(m_mutex);
			m_closed = true;
			m_deliverCond.notify_all();
		}

	private:
		Mutex m_mutex;
		ConditionVariable m_waitCond;
		ConditionVariable m_deliverCond;
		Event m_event;
		bool m_closed = false;
	};

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

	class SelectCaseDesc;

	template<typename T, ChanDir dir = ChanDir::Both>
	class Chan
	{
		friend class SelectCaseDesc;

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
		Map<SelectCond*, size_t> m_writeSelects;
		Map<SelectCond*, size_t> m_readSelects;
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
			  m_writeSelects(allocator),
			  m_readSelects(allocator),
			  m_buffer(allocator),
			  m_readMutex(allocator),
			  m_writeMutex(allocator)
		{}

		void signalSelectAndRemove(Map<SelectCond*, size_t>& selects)
		{
			if (selects.count() == 0)
				return;

			std::random_device device;
			std::uniform_int_distribution<size_t> dist(0, selects.count() - 1);
			auto offsetIndex = dist(device);
			auto it = selects.begin();
			for (size_t i = 0; i < selects.count(); ++i)
			{
				auto index = (i + offsetIndex) % selects.count();
				if (it[index].key->trySignalReady(it[index].value))
				{
					selects.remove(it[index].key);
					return;
				}
			}
			while (selects.count() > 0)
			{
				auto it = selects.begin();
				if (it[offsetIndex].key->signalReady(it[offsetIndex].value) == false)
					selects.remove(it[offsetIndex].key);
			}
		}

		ChanErr internalSend(T* ptr)
		{
			if (isBuffered())
			{
				auto lockMutex = lockGuard(m_mutex);
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
				{
					m_readCond.notify_one();
				}
				else if (m_readSelects.count() > 0)
				{
					signalSelectAndRemove(m_readSelects);
				}

				return ChanErr::Ok;
			}
			else
			{
				auto lockWriteMutex = lockGuard(m_writeMutex);
				auto lockMutex = lockGuard(m_mutex);

				if (m_closed.load())
					return ChanErr::Closed;

				m_unbufferedSlot = ptr;
				m_writeWaiting.fetch_add(1);
				if (m_readWaiting.load() > 0)
				{
					m_readCond.notify_one();
				}
				else if (m_readSelects.count() > 0)
				{
					signalSelectAndRemove(m_readSelects);
				}
				m_writeCond.wait(m_mutex);
				return ChanErr::Ok;
			}
		}

		Result<T, ChanErr> internalRecv()
		{
			if (isBuffered())
			{
				auto lockMutex = lockGuard(m_mutex);
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
				{
					m_writeCond.notify_one();
				}
				else if (m_writeSelects.count() > 0)
				{
					signalSelectAndRemove(m_writeSelects);
				}
				return res;
			}
			else
			{
				auto lockReadMutex = lockGuard(m_readMutex);
				auto lockMutex = lockGuard(m_mutex);

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
				if (m_writeSelects.count() > 0)
				{
					signalSelectAndRemove(m_writeSelects);
				}
				return res;
			}
		}

		void internalInsertReadSelectCond(SelectCond* cond, size_t index)
		{
			if (cond == nullptr)
				return;
			m_readSelects.insert(cond, index);
		}

		void internalInsertWriteSelectCond(SelectCond* cond, size_t index)
		{
			if (cond == nullptr)
				return;
			m_writeSelects.insert(cond, index);
		}

		ChanErr internalTrySend(T* ptr, SelectCond* cond, size_t index)
		{
			if (isBuffered())
			{
				auto lockMutex = lockGuard(m_mutex);
				if (m_buffer.count() == m_bufferSize)
				{
					internalInsertWriteSelectCond(cond, index);
					return ChanErr::Empty;
				}

				coreAssert(m_buffer.count() < m_bufferSize);
				if constexpr (std::is_move_constructible_v<T>)
					m_buffer.push_back(std::move(*ptr));
				else
					m_buffer.push_back(*ptr);
				if (m_readWaiting.load() > 0)
				{
					m_readCond.notify_one();
				}
				else if (m_readSelects.count() > 0)
				{
					signalSelectAndRemove(m_readSelects);
				}
				internalInsertWriteSelectCond(cond, index);
				return ChanErr::Ok;
			}
			else
			{
				auto lockWriteMutex = tryLockGuard(m_writeMutex);
				if (lockWriteMutex.is_locked() == false)
				{
					internalInsertWriteSelectCond(cond, index);
					return ChanErr::Empty;
				}
				auto lockMutex = lockGuard(m_mutex);

				if (m_closed.load())
				{
					// internalInsertWriteSelectCond(cond, index);
					return ChanErr::Closed;
				}

				m_unbufferedSlot = ptr;
				m_writeWaiting.fetch_add(1);
				if (m_readWaiting.load() > 0)
				{
					m_readCond.notify_one();
				}
				else if (m_readSelects.count() > 0)
				{
					signalSelectAndRemove(m_readSelects);
				}
				m_writeCond.wait(m_mutex);
				internalInsertWriteSelectCond(cond, index);
				return ChanErr::Ok;
			}
		}

		Result<T, ChanErr> internalTryRecv(SelectCond* cond, size_t index)
		{
			if (isBuffered())
			{
				auto lockMutex = lockGuard(m_mutex);
				if (m_buffer.count() == 0)
				{
					internalInsertReadSelectCond(cond, index);
					return ChanErr::Empty;
				}

				coreAssert(m_buffer.count() > 0);
				auto res = Result<T, ChanErr>::createEmpty();
				if constexpr (std::is_move_constructible_v<T>)
					res = std::move(m_buffer.front());
				else
					res = m_buffer.front();
				m_buffer.pop_front();

				if (m_writeWaiting.load() > 0)
				{
					m_writeCond.notify_one();
				}
				else if (m_writeSelects.count() > 0)
				{
					signalSelectAndRemove(m_writeSelects);
				}
				internalInsertReadSelectCond(cond, index);
				return res;
			}
			else
			{
				auto lockReadMutex = lockGuard(m_readMutex);
				auto lockMutex = lockGuard(m_mutex);

				if (m_closed.load())
				{
					// internalInsertReadSelectCond(cond, index);
					return ChanErr::Closed;
				}
				else if (m_writeWaiting.load() == 0)
				{
					internalInsertReadSelectCond(cond, index);
					return ChanErr::Empty;
				}

				auto res = Result<T, ChanErr>::createEmpty();
				if constexpr (std::is_move_constructible_v<T>)
					res = std::move(*m_unbufferedSlot);
				else
					res = *m_unbufferedSlot;
				m_unbufferedSlot = nullptr;
				m_writeWaiting.fetch_sub(1);
				m_writeCond.notify_one();
				if (m_writeSelects.count() > 0)
				{
					signalSelectAndRemove(m_writeSelects);
				}
				internalInsertReadSelectCond(cond, index);
				return res;
			}
		}

		bool internalClose()
		{
			auto lockMutex = lockGuard(m_mutex);
			if (m_closed.load())
				return false;
			m_closed.store(true);
			m_readCond.notify_all();
			m_writeCond.notify_all();
			for (auto& [cond, index]: m_readSelects)
				cond->signalClose(index);
			for (auto& [cond, index]: m_writeSelects)
				cond->signalClose(index);
			return true;
		}

		void internalRemoveSelectCond(SelectCond* cond)
		{
			if (cond == nullptr)
				return;
			auto lock = lockGuard(m_mutex);
			m_readSelects.remove(cond);
			m_writeSelects.remove(cond);
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
				auto lockMutex = lockGuard(m_mutex);
				return m_buffer.count();
			}
			return 0;
		}

		bool isClosed()
		{
			auto lockMutex = lockGuard(m_mutex);
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
			return internalTrySend(&value, nullptr, 0);
		}

		Result<T, ChanErr> tryRecv()
		{
			return internalTryRecv(nullptr, 0);
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
		ChanErr (*m_eval)(void* ptr, SelectCond* cond, size_t index) = nullptr;
		ChanErr (*m_eval2)(void* ptr) = nullptr;
		void (*m_removeSelectCond)(void* ptr, SelectCond* cond) = nullptr;
		bool m_isDefault = false;
	public:
		template<typename T, ChanDir dir>
		SelectCaseDesc(ReadCase<T, dir>& c)
			: m_case(&c)
		{
			m_eval = +[](void* ptr, SelectCond* cond, size_t index) {
				auto c = (ReadCase<T, dir>*)ptr;
				c->m_res = c->m_chan->internalTryRecv(cond, index);
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

			m_eval2 = +[](void* ptr) {
				auto c = (ReadCase<T, dir>*)ptr;
				c->m_res = c->m_chan->recv();
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

			m_removeSelectCond = +[](void* ptr, SelectCond* cond) {
				auto c = (ReadCase<T, dir>*)ptr;
				c->m_chan->internalRemoveSelectCond(cond);
			};
		}

		template<typename T, ChanDir dir>
		SelectCaseDesc(WriteCase<T, dir>& c)
			: m_case(&c)
		{
			m_eval = +[](void* ptr, SelectCond* cond, size_t index) {
				auto c = (WriteCase<T, dir>*)ptr;
				auto err = c->m_chan->internalTrySend(c->m_value, cond, index);
				if (err == ChanErr::Ok)
					c->m_callback();
				return err;
			};

			m_eval2 = +[](void* ptr) {
				auto c = (WriteCase<T, dir>*)ptr;
				auto err = c->m_chan->send(c->m_value);
				if (err == ChanErr::Ok)
					c->m_callback();
				return err;
			};

			m_removeSelectCond = +[](void* ptr, SelectCond* cond) {
				auto c = (WriteCase<T, dir>*)ptr;
				c->m_chan->internalRemoveSelectCond(cond);
			};
		}

		SelectCaseDesc(DefaultCase& c)
			: m_case(&c),
			  m_isDefault(true)
		{
			m_eval = +[](void* ptr, SelectCond*, size_t) {
				auto c = (DefaultCase*)ptr;
				c->m_callback();
				return ChanErr::Ok;
			};

			m_eval2 = +[](void* ptr) {
				auto c = (DefaultCase*)ptr;
				c->m_callback();
				return ChanErr::Ok;
			};

			m_removeSelectCond = +[](void*, SelectCond*) {};
		}

		bool isDefault() const { return m_isDefault; }
		ChanErr eval(SelectCond* cond, size_t index)
		{
			return m_eval(m_case, cond, index);
		}
		ChanErr eval2()
		{
			return m_eval2(m_case);
		}
		void removeCond(SelectCond* cond)
		{
			m_removeSelectCond(m_case, cond);
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

			if (hasDefault == false)
			{
				Mallocator mallocator;
				SelectCond cond{&mallocator};

				std::random_device device;
				std::uniform_int_distribution<size_t> dist(0, descsCount);
				auto offsetIndex = dist(device);
				auto aliveDescsCount = descsCount;
				for (size_t i = 0; i < descsCount; ++i)
				{
					auto index = (i + offsetIndex) % descsCount;
					auto err = descs[index].eval(&cond, index);
					if (err == ChanErr::Ok)
					{
						cond.close();
						for (size_t j = 0; j <= i; ++j)
						{
							auto index = (j + offsetIndex) % descsCount;
							descs[index].removeCond(&cond);
						}
						return;
					}
					else if (err == ChanErr::Closed)
					{
						--aliveDescsCount;
					}
				}


				while (aliveDescsCount > 0)
				{
					auto event = cond.waitForEventAndClose();
					coreAssert(event.isSignaled());
					if (event.closed)
					{
						descs[event.index].removeCond(&cond);
						--aliveDescsCount;
					}
					else
					{
						auto err = descs[event.index].eval2();
						if (err == ChanErr::Ok)
						{
							for (size_t i = 0; i < descsCount; ++i)
								descs[i].removeCond(&cond);
							return;
						}
						else if (err == ChanErr::Closed)
						{
							descs[event.index].removeCond(&cond);
							--aliveDescsCount;
						}
					}
				}
			}
			else
			{
				std::random_device device;
				std::uniform_int_distribution<size_t> dist(0, descsCount);
				auto offsetIndex = dist(device);
				for (size_t i = 0; i < descsCount; ++i)
				{
					auto index = (i + offsetIndex) % descsCount;
					if (index == defaultIndex)
						continue;
					auto err = descs[index].eval(nullptr, 0);
					if (err == ChanErr::Ok)
						return;
				}
				descs[defaultIndex].eval(nullptr, 0);
				return;
			}
		}
	};
}
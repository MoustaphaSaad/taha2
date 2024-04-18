#pragma once

#include "core/Allocator.h"
#include "core/HashFunction.h"
#include "core/Assert.h"

#include <cstdint>
#include <cstring>
#include <new>
#include <utility>

namespace core
{
	enum HASH_FLAGS: uint8_t { HASH_EMPTY, HASH_USED, HASH_DELETED };

	// hash table slot with the index and hash
	class HashSlot
	{
		// most signficant 2 bits = HASH_FLAGS enum
		// remaining bits = index
		size_t index = 0;
		size_t hash = 0;
	public:
		HashSlot() = default;

		HashSlot(size_t value_index, size_t value_hash)
			:index(value_index),
			 hash(value_hash)
		{}

		// returns the flags of the given hash slot
		HASH_FLAGS flags() const
		{
			if constexpr (sizeof(size_t) == 8)
				return HASH_FLAGS((index & 0xC000000000000000) >> 62);
			else if constexpr (sizeof(size_t) == 4)
				return HASH_FLAGS((index & 0xC0000000) >> 30);
			else
				static_assert("unexpected sizeof(size_t)");
		}

		// returns the index into values array of the given hash slot
		size_t valueIndex() const
		{
			if constexpr (sizeof(size_t) == 8)
				return size_t(index & 0x3FFFFFFFFFFFFFFF);
			else if constexpr (sizeof(size_t) == 4)
				return size_t(index & 0x3FFFFFFF);
			else
				static_assert("unexpected sizeof(size_t)");
		}

		size_t valueHash() const
		{
			return hash;
		}

		// sets the flags of the given hash slot
		void setFlags(HASH_FLAGS f)
		{
			if constexpr (sizeof(size_t) == 8)
			{
				index &= ~0xC000000000000000;
				index |= (size_t(f) << 62);
			}
			else if constexpr (sizeof(size_t) == 4)
			{
				index &= ~0xC0000000;
				index |= (size_t(f) << 30);
			}
			else
			{
				static_assert("unexpected sizeof(size_t)");
			}
		}

		// sets the index in values array of the given hash slot
		void setValueIndex(size_t value_index)
		{
			if constexpr (sizeof(size_t) == 8)
			{
				index &= ~0x3FFFFFFFFFFFFFFF;
				index |= (size_t(value_index) & 0x3FFFFFFFFFFFFFFF);
			}
			else if constexpr (sizeof(size_t) == 4)
			{
				index &= ~0x3FFFFFFF;
				index |= (size_t(value_index) & 0x3FFFFFFF);
			}
			else
			{
				static_assert("unexpected sizeof(size_t)");
			}
		}

		// sets the hash of the value
		void setValueHash(size_t value_hash)
		{
			hash = value_hash;
		}
	};

	template<typename T, typename THash = Hash<T>>
	class Set
	{
		Allocator* m_allocator = nullptr;
		HashSlot* m_slots = nullptr;
		size_t m_slotsCount = 0;
		size_t m_deletedCount = 0;
		size_t m_usedCountThreshold = 0;
		size_t m_usedCountShrinkThreshold = 0;
		size_t m_deletedCountThreshold = 0;
		T* m_values = nullptr;
		size_t m_valuesCount = 0;
		size_t m_valuesCapacity = 0;

		struct Search_Result
		{
			size_t hash;
			size_t index;
		};

		void destroy()
		{
			for (size_t i = 0; i < m_valuesCount; ++i)
				m_values[i].~T();
			m_allocator->release(m_values, sizeof(T) * m_valuesCapacity);
			m_allocator->free(m_values, sizeof(T) * m_valuesCapacity);

			m_allocator->release(m_slots, sizeof(HashSlot) * m_slotsCount);
			m_allocator->free(m_slots, sizeof(HashSlot) * m_slotsCount);

			m_slots = nullptr;
			m_slotsCount = 0;
			m_deletedCount = 0;
			m_usedCountThreshold = 0;
			m_usedCountShrinkThreshold = 0;
			m_deletedCountThreshold = 0;
			m_values = nullptr;
			m_valuesCount = 0;
			m_valuesCapacity = 0;
		}

		void copyFrom(const Set& other)
		{
			m_allocator = other.m_allocator;
			m_slotsCount = other.count;
			m_deletedCount = other.m_deletedCount;
			m_usedCountThreshold = other.m_usedCountThreshold;
			m_usedCountShrinkThreshold = other.m_usedCountShrinkThreshold;
			m_deletedCountThreshold = other.m_deletedCountThreshold;
			m_valuesCount = other.m_valuesCount;
			m_valuesCapacity = m_valuesCount;

			m_values = (T*)m_allocator->alloc(sizeof(T) * m_valuesCount, alignof(T));
			m_allocator->commit(m_values, sizeof(T) * m_valuesCount);
			for (size_t i = 0; i < m_valuesCount; ++i)
				::new (m_values + i) T(other.m_values[i]);

			m_slots = (HashSlot*)m_allocator->alloc(sizeof(HashSlot) * m_slotsCount, alignof(HashSlot));
			m_allocator->commit(m_slots, sizeof(HashSlot) * m_slotsCount);
			for (size_t i = 0; i < m_slotsCount; ++i)
				::new (m_slots + i) HashSlot();

			for (size_t i = 0; i < m_valuesCount; ++i)
			{
				auto res = findSlotForInsert(m_slots, m_slotsCount, m_values, m_values[i]);
				auto& slot = m_slots[res.index];
				auto flags = slot.flags();
				switch (flags)
				{
				case HASH_FLAGS::HASH_EMPTY:
				{
					slot.setFlags(HASH_FLAGS::HASH_USED);
					slot.setValueIndex(i);
					slot.setValueHash(res.hash);
					break;
				}
				case HASH_FLAGS::HASH_USED:
				case HASH_FLAGS::HASH_DELETED:
				default:
					coreUnreachable();
					break;
				}
			}
		}

		void moveFrom(Set&& other)
		{
			m_allocator = other.m_allocator;
			m_slots = other.m_slots;
			m_slotsCount = other.m_slotsCount;
			m_deletedCount = other.m_deletedCount;
			m_usedCountThreshold = other.m_usedCountThreshold;
			m_usedCountShrinkThreshold = other.m_usedCountShrinkThreshold;
			m_deletedCountThreshold = other.m_deletedCountThreshold;
			m_values = other.m_values;
			m_valuesCount = other.m_valuesCount;
			m_valuesCapacity = other.m_valuesCapacity;

			other.m_slots = nullptr;
			other.m_slotsCount = 0;
			other.m_deletedCount = 0;
			other.m_usedCountThreshold = 0;
			other.m_usedCountShrinkThreshold = 0;
			other.m_deletedCountThreshold = 0;
			other.m_values = nullptr;
			other.m_valuesCount = 0;
			other.m_valuesCapacity = 0;
		}

		Search_Result
		findSlotForInsert(const HashSlot* _slots, size_t _slots_count, const T* _values, const T& key)
		{
			Search_Result res{};
			res.hash = THash{}(key, size_t(_slots));

			auto cap = _slots_count;
			if (cap == 0) return res;

			auto index = res.hash & (cap - 1);
			auto ix = index;

			size_t first_deleted_slot_index = 0;
			bool found_first_deleted_slot = false;

			// linear probing
			while (true)
			{
				const auto& slot = _slots[ix];
				auto slot_hash = slot.valueHash();
				auto slot_index = slot.valueIndex();
				auto slot_flags = slot.flags();
				switch (slot_flags)
				{
				// this position is not empty but if it's the same value then we return it
				case HASH_USED:
				{
					if (slot_hash == res.hash && _values[slot_index] == key)
					{
						res.index = ix;
						return res;
					}
					break;
				}
				// this is an empty slot then we can use it
				case HASH_EMPTY:
				{
					// we didn't find the key, so check the first deleted slot if we found one then
					// reuse it
					if (found_first_deleted_slot)
					{
						res.index = first_deleted_slot_index;
					}
					// we didn't find the key, so use the first empty slot
					else
					{
						res.index = ix;
					}
					return res;
				}
				case HASH_DELETED:
				{
					if (found_first_deleted_slot == false)
					{
						first_deleted_slot_index = ix;
						found_first_deleted_slot = true;
					}
					break;
				}
				default:
					coreUnreachable();
					return {};
				}

				// the position is not empty and the key is not the same
				++ix;
				ix &= (cap - 1);

				// if we went full circle then we just return the cap to signal no index has been found
				if (ix == index)
				{
					ix = cap;
					break;
				}
			}

			res.index = ix;
			return res;
		}

		Search_Result findSlotForLookup(const T& key) const
		{
			Search_Result res{};
			res.hash = THash{}(key, size_t(m_slots));

			auto cap = m_slotsCount;
			if (cap == 0) return res;

			auto index = res.hash & (cap - 1);
			auto ix = index;

			// linear probing
			while(true)
			{
				const auto& slot = m_slots[ix];
				auto slot_hash = slot.valueHash();
				auto slot_index = slot.valueIndex();
				auto slot_flags = slot.flags();

				// if the cell is empty then we didn't find the value
				if (slot_flags == HASH_FLAGS::HASH_EMPTY)
				{
					ix = cap;
					break;
				}

				// if the cell is used and it's the same value then we found it
				if (slot_flags == HASH_FLAGS::HASH_USED &&
					slot_hash == res.hash &&
					m_values[slot_index] == key)
				{
					break;
				}

				// the position is not used or the key is not the same
				++ix;
				ix &= (cap - 1);

				// if we went full circle then we just return the cap to signal no index has been found
				if (ix == index)
				{
					ix = cap;
					break;
				}
			}

			res.index = ix;
			return res;
		}

		void reserveExact(size_t new_count)
		{
			auto new_slots = (HashSlot*)m_allocator->alloc(sizeof(HashSlot) * new_count, alignof(HashSlot));
			m_allocator->commit(new_slots, sizeof(HashSlot) * new_count);
			for (size_t i = 0; i < new_count; ++i)
				::new (new_slots + i) HashSlot();

			m_deletedCount = 0;
			// if 12/16th of table is occupied, grow
			m_usedCountThreshold = new_count - (new_count >> 2);
			// if deleted count is 3/16th of table, rebuild
			m_deletedCountThreshold = (new_count >> 3) + (new_count >> 4);
			// if table is only 4/16th full, shrink
			m_usedCountShrinkThreshold = new_count >> 2;

			// do a rehash
			if (m_valuesCount != 0)
			{
				for (size_t i = 0; i < m_valuesCount; ++i)
				{
					auto res = findSlotForInsert(new_slots, new_count, m_values, m_values[i]);
					auto& slot = new_slots[res.index];
					auto flags = slot.flags();
					switch (flags)
					{
					case HASH_FLAGS::HASH_EMPTY:
					{
						slot.setFlags(HASH_FLAGS::HASH_USED);
						slot.setValueIndex(i);
						slot.setValueHash(res.hash);
						break;
					}
					case HASH_FLAGS::HASH_USED:
					case HASH_FLAGS::HASH_DELETED:
					default:
						coreUnreachable();
						break;
					}
				}
			}

			m_allocator->release(m_slots, sizeof(HashSlot) * m_slotsCount);
			m_allocator->free(m_slots, sizeof(HashSlot) * m_slotsCount);
			m_slots = new_slots;
			m_slotsCount = new_count;
		}

		void maintainSpaceComplexity()
		{
			if (m_slotsCount == 0)
			{
				reserveExact(8);
			}
			else if (m_valuesCount + 1 > m_usedCountThreshold)
			{
				reserveExact(m_slotsCount * 2);
			}
		}

		void valuesGrow(size_t new_capacity)
		{
			auto new_ptr = (T*)m_allocator->alloc(sizeof(T) * new_capacity, alignof(T));
			m_allocator->commit(new_ptr, sizeof(T) * m_valuesCount);
			for (size_t i = 0; i < m_valuesCount; ++i)
			{
				if constexpr (std::is_move_constructible_v<T>)
					::new (new_ptr + i) T(std::move(m_values[i]));
				else
					::new (new_ptr + i) T(m_values[i]);
				m_values[i].~T();
			}

			m_allocator->release(m_values, sizeof(T) * m_valuesCapacity);
			m_allocator->free(m_values, sizeof(T) * m_valuesCapacity);

			m_values = new_ptr;
			m_valuesCapacity = new_capacity;
		}

		void valuesEnsureSpaceExists(size_t i = 1)
		{
			if (m_valuesCount + i > m_valuesCapacity)
			{
				size_t new_capacity = m_valuesCapacity * 2;
				if (new_capacity == 0)
					new_capacity = 8;

				if (new_capacity < m_valuesCapacity + i)
					new_capacity = m_valuesCapacity + i;

				valuesGrow(new_capacity);
			}
		}

		template<typename R>
		void valuesPush(R&& key)
		{
			valuesEnsureSpaceExists();
			m_allocator->commit(m_values + m_valuesCount, sizeof(T));
			::new (m_values + m_valuesCount) T(std::forward<R>(key));
			++m_valuesCount;
		}

		void valuesShrinkToFit()
		{
			valuesGrow(m_valuesCount);
		}

	public:
		using iterator = T*;

		explicit Set(Allocator* a)
			: m_allocator(a)
		{}

		Set(const Set& other)
		{
			copyFrom(other);
		}

		Set(Set&& other)
		{
			moveFrom(std::move(other));
		}

		Set& operator=(const Set& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		Set& operator=(Set&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~Set()
		{
			destroy();
		}

		template<typename R>
		void insert(R&& key)
		{
			maintainSpaceComplexity();

			auto res = findSlotForInsert(m_slots, m_slotsCount, m_values, key);

			auto& slot = m_slots[res.index];
			auto flags = slot.flags();
			switch (flags)
			{
			case HASH_EMPTY:
			{
				slot.setFlags(HASH_USED);
				slot.setValueIndex(m_valuesCount);
				slot.setValueHash(res.hash);
				valuesPush(std::forward<R>(key));
				return;
			}
			case HASH_DELETED:
			{
				slot.setFlags(HASH_USED);
				slot.setValueIndex(m_valuesCount);
				slot.setValueHash(res.hash);
				--m_deletedCount;
				valuesPush(std::forward<R>(key));
			}
			case HASH_USED:
			{
				return;
			}
			default:
			{
				coreUnreachable();
				return;
			}
			}
		}

		void clear()
		{
			for (size_t i = 0; i < m_slotsCount; ++i)
				m_slots[i] = HashSlot{};
			for (size_t i = 0; i < m_valuesCount; ++i)
				m_values[i].~T();
			m_valuesCount = 0;
			m_deletedCount = 0;
		}

		Allocator* allocator() const { return m_allocator; }

		size_t count() const
		{
			return m_valuesCount;
		}

		size_t capacity() const
		{
			return m_slotsCount;
		}

		void reserve(size_t added_count)
		{
			auto new_cap = m_valuesCount + added_count;
			new_cap *= 4;
			new_cap = new_cap / 3 + 1;
			if (new_cap > m_usedCountThreshold)
			{
				// round up to next power of 2
				if (new_cap > 0)
				{
					--new_cap;
					if constexpr (sizeof(size_t) == 4)
					{
						new_cap |= new_cap >> 1;
						new_cap |= new_cap >> 2;
						new_cap |= new_cap >> 4;
						new_cap |= new_cap >> 8;
						new_cap |= new_cap >> 16;
					}
					else if constexpr (sizeof(size_t) == 8)
					{
						new_cap |= new_cap >> 1;
						new_cap |= new_cap >> 2;
						new_cap |= new_cap >> 4;
						new_cap |= new_cap >> 8;
						new_cap |= new_cap >> 16;
						new_cap |= new_cap >> 32;
					}
					else
					{
						static_assert("unexpected sizeof(size_t)");
					}
					++new_cap;
				}
				reserveExact(new_cap);
			}
		}

		template<typename R>
		const iterator lookup(const R& key) const
		{
			auto res = findSlotForLookup(key);
			if (res.index == m_slotsCount)
				return nullptr;
			auto& slot = m_slots[res.index];
			auto index = slot.valueIndex();
			return m_values + index;
		}

		template<typename R>
		bool remove(const R& key)
		{
			auto res = findSlotForLookup(key);
			if (res.index == m_slotsCount)
				return false;
			auto& slot = m_slots[res.index];
			auto index = slot.valueIndex();
			slot.setFlags(HASH_DELETED);

			m_values[index].~T();

			if (index < m_valuesCount - 1)
			{
				// fixup the index of the last element after swap
				auto last_res = findSlotForLookup(m_values[m_valuesCount - 1]);
				m_slots[last_res.index].setValueIndex(index);
				::new (m_values + index) T(std::move_if_noexcept(m_values[m_valuesCount - 1]));
				m_values[m_valuesCount - 1].~T();
			}

			--m_valuesCount;
			++m_deletedCount;

			// rehash because of size is too low
			if (m_valuesCount < m_usedCountShrinkThreshold && m_slotsCount > 8)
			{
				reserveExact(m_slotsCount >> 1);
				valuesShrinkToFit();
			}
			// rehash because of too many deleted values
			else if (m_deletedCount > m_deletedCountThreshold)
			{
				reserveExact(m_slotsCount);
			}
			return true;
		}

		const iterator begin() const
		{
			return m_values;
		}

		const iterator end() const
		{
			return m_values + m_valuesCount;
		}
	};

	template<typename TKey, typename TValue>
	struct KeyValue
	{
		TKey key;
		TValue value;
	};

	template<typename TKey, typename TValue, typename THash = Hash<TKey>>
	class Map
	{
		Allocator* m_allocator = nullptr;
		HashSlot* m_slots = nullptr;
		size_t m_slotsCount = 0;
		size_t m_deletedCount = 0;
		size_t m_usedCountThreshold = 0;
		size_t m_usedCountShrinkThreshold = 0;
		size_t m_deletedCountThreshold = 0;
		KeyValue<const TKey, TValue>* m_values = nullptr;
		size_t m_valuesCount = 0;
		size_t m_valuesCapacity = 0;

		struct Search_Result
		{
			size_t hash;
			size_t index;
		};

		void destroy()
		{
			for (size_t i = 0; i < m_valuesCount; ++i)
				m_values[i].~KeyValue();
			m_allocator->release(m_values, sizeof(KeyValue<TKey, TValue>) * m_valuesCapacity);
			m_allocator->free(m_values, sizeof(KeyValue<TKey, TValue>) * m_valuesCapacity);

			m_allocator->release(m_slots, sizeof(HashSlot) * m_slotsCount);
			m_allocator->free(m_slots, sizeof(HashSlot) * m_slotsCount);

			m_slots = nullptr;
			m_slotsCount = 0;
			m_deletedCount = 0;
			m_usedCountThreshold = 0;
			m_usedCountShrinkThreshold = 0;
			m_deletedCountThreshold = 0;
			m_values = nullptr;
			m_valuesCount = 0;
			m_valuesCapacity = 0;
		}

		void copyFrom(const Map& other)
		{
			m_allocator = other.m_allocator;
			m_slotsCount = other.m_slotsCount;
			m_deletedCount = other.m_deletedCount;
			m_usedCountThreshold = other.m_usedCountThreshold;
			m_usedCountShrinkThreshold = other.m_usedCountShrinkThreshold;
			m_deletedCountThreshold = other.m_deletedCountThreshold;
			m_valuesCount = other.m_valuesCount;
			m_valuesCapacity = m_valuesCount;

			m_values = (KeyValue<const TKey, TValue>*)m_allocator->alloc(sizeof(KeyValue<const TKey, TValue>) * m_valuesCount, alignof(KeyValue<const TKey, TValue>));
			m_allocator->commit(m_values, sizeof(TValue) * m_valuesCount);
			for (size_t i = 0; i < m_valuesCount; ++i)
				::new (m_values + i) KeyValue<const TKey, TValue>(other.m_values[i]);

			m_slots = (HashSlot*)m_allocator->alloc(sizeof(HashSlot) * m_slotsCount, alignof(HashSlot));
			m_allocator->commit(m_slots, sizeof(HashSlot) * m_slotsCount);
			for (size_t i = 0; i < m_slotsCount; ++i)
				::new (m_slots + i) HashSlot();

			for (size_t i = 0; i < m_valuesCount; ++i)
			{
				auto res = findSlotForInsert(m_slots, m_slotsCount, m_values, m_values[i].key);
				auto& slot = m_slots[res.index];
				auto flags = slot.flags();
				switch (flags)
				{
				case HASH_FLAGS::HASH_EMPTY:
				{
					slot.setFlags(HASH_FLAGS::HASH_USED);
					slot.setValueIndex(i);
					slot.setValueHash(res.hash);
					break;
				}
				case HASH_FLAGS::HASH_USED:
				case HASH_FLAGS::HASH_DELETED:
				default:
					coreUnreachable();
					break;
				}
			}
		}

		void moveFrom(Map&& other)
		{
			m_allocator = other.m_allocator;
			m_slots = other.m_slots;
			m_slotsCount = other.m_slotsCount;
			m_deletedCount = other.m_deletedCount;
			m_usedCountThreshold = other.m_usedCountThreshold;
			m_usedCountShrinkThreshold = other.m_usedCountShrinkThreshold;
			m_deletedCountThreshold = other.m_deletedCountThreshold;
			m_values = other.m_values;
			m_valuesCount = other.m_valuesCount;
			m_valuesCapacity = other.m_valuesCapacity;

			other.m_slots = nullptr;
			other.m_slotsCount = 0;
			other.m_deletedCount = 0;
			other.m_usedCountThreshold = 0;
			other.m_usedCountShrinkThreshold = 0;
			other.m_deletedCountThreshold = 0;
			other.m_values = nullptr;
			other.m_valuesCount = 0;
			other.m_valuesCapacity = 0;
		}

		Search_Result
		findSlotForInsert(const HashSlot* _slots, size_t _slots_count, const KeyValue<const TKey, TValue>* _values, const TKey& key)
		{
			Search_Result res{};
			res.hash = THash{}(key, size_t(_slots));

			auto cap = _slots_count;
			if (cap == 0) return res;

			auto index = res.hash & (cap - 1);
			auto ix = index;

			size_t first_deleted_slot_index = 0;
			bool found_first_deleted_slot = false;

			// linear probing
			while (true)
			{
				const auto& slot = _slots[ix];
				auto slot_hash = slot.valueHash();
				auto slot_index = slot.valueIndex();
				auto slot_flags = slot.flags();
				switch (slot_flags)
				{
				// this position is not empty but if it's the same value then we return it
				case HASH_USED:
				{
					if (slot_hash == res.hash && _values[slot_index].key == key)
					{
						res.index = ix;
						return res;
					}
					break;
				}
				// this is an empty slot then we can use it
				case HASH_EMPTY:
				{
					// we didn't find the key, so check the first deleted slot if we found one then
					// reuse it
					if (found_first_deleted_slot)
					{
						res.index = first_deleted_slot_index;
					}
					// we didn't find the key, so use the first empty slot
					else
					{
						res.index = ix;
					}
					return res;
				}
				case HASH_DELETED:
				{
					if (found_first_deleted_slot == false)
					{
						first_deleted_slot_index = ix;
						found_first_deleted_slot = true;
					}
					break;
				}
				default:
					coreUnreachable();
					return {};
				}

				// the position is not empty and the key is not the same
				++ix;
				ix &= (cap - 1);

				// if we went full circle then we just return the cap to signal no index has been found
				if (ix == index)
				{
					ix = cap;
					break;
				}
			}

			res.index = ix;
			return res;
		}

		Search_Result findSlotForLookup(const TKey& key) const
		{
			Search_Result res{};
			res.hash = THash{}(key, size_t(m_slots));

			auto cap = m_slotsCount;
			if (cap == 0) return res;

			auto index = res.hash & (cap - 1);
			auto ix = index;

			// linear probing
			while(true)
			{
				const auto& slot = m_slots[ix];
				auto slot_hash = slot.valueHash();
				auto slot_index = slot.valueIndex();
				auto slot_flags = slot.flags();

				// if the cell is empty then we didn't find the value
				if (slot_flags == HASH_FLAGS::HASH_EMPTY)
				{
					ix = cap;
					break;
				}

				// if the cell is used and it's the same value then we found it
				if (slot_flags == HASH_FLAGS::HASH_USED &&
					slot_hash == res.hash &&
					m_values[slot_index].key == key)
				{
					break;
				}

				// the position is not used or the key is not the same
				++ix;
				ix &= (cap - 1);

				// if we went full circle then we just return the cap to signal no index has been found
				if (ix == index)
				{
					ix = cap;
					break;
				}
			}

			res.index = ix;
			return res;
		}

		void reserveExact(size_t new_count)
		{
			auto new_slots = (HashSlot*)m_allocator->alloc(sizeof(HashSlot) * new_count, alignof(HashSlot));
			m_allocator->commit(new_slots, sizeof(HashSlot) * new_count);
			for (size_t i = 0; i < new_count; ++i)
				::new (new_slots + i) HashSlot();

			m_deletedCount = 0;
			// if 12/16th of table is occupied, grow
			m_usedCountThreshold = new_count - (new_count >> 2);
			// if deleted count is 3/16th of table, rebuild
			m_deletedCountThreshold = (new_count >> 3) + (new_count >> 4);
			// if table is only 4/16th full, shrink
			m_usedCountShrinkThreshold = new_count >> 2;

			// do a rehash
			if (m_valuesCount != 0)
			{
				for (size_t i = 0; i < m_valuesCount; ++i)
				{
					auto res = findSlotForInsert(new_slots, new_count, m_values, m_values[i].key);
					auto& slot = new_slots[res.index];
					auto flags = slot.flags();
					switch (flags)
					{
					case HASH_FLAGS::HASH_EMPTY:
					{
						slot.setFlags(HASH_FLAGS::HASH_USED);
						slot.setValueIndex(i);
						slot.setValueHash(res.hash);
						break;
					}
					case HASH_FLAGS::HASH_USED:
					case HASH_FLAGS::HASH_DELETED:
					default:
						coreUnreachable();
						break;
					}
				}
			}

			m_allocator->release(m_slots, sizeof(HashSlot) * m_slotsCount);
			m_allocator->free(m_slots, sizeof(HashSlot) * m_slotsCount);
			m_slots = new_slots;
			m_slotsCount = new_count;
		}

		void maintainSpaceComplexity()
		{
			if (m_slotsCount == 0)
			{
				reserveExact(8);
			}
			else if (m_valuesCount + 1 > m_usedCountThreshold)
			{
				reserveExact(m_slotsCount * 2);
			}
		}

		void valuesGrow(size_t new_capacity)
		{
			auto new_ptr = (KeyValue<const TKey, TValue>*)m_allocator->alloc(sizeof(KeyValue<const TKey, TValue>) * new_capacity, alignof(KeyValue<const TKey, TValue>));
			m_allocator->commit(new_ptr, sizeof(*m_values) * m_valuesCount);
			for (size_t i = 0; i < m_valuesCount; ++i)
			{
				if constexpr (std::is_move_constructible_v<KeyValue<const TKey, TValue>>)
					::new (new_ptr + i) KeyValue<const TKey, TValue>(std::move(m_values[i]));
				else
					::new (new_ptr + i) KeyValue<const TKey, TValue>(m_values[i]);
				m_values[i].~KeyValue();
			}

			m_allocator->release(m_values, sizeof(*m_values) * m_valuesCapacity);
			m_allocator->free(m_values, sizeof(*m_values) * m_valuesCapacity);

			m_values = new_ptr;
			m_valuesCapacity = new_capacity;
		}

		void valuesEnsureSpaceExists(size_t i = 1)
		{
			if (m_valuesCount + i > m_valuesCapacity)
			{
				size_t new_capacity = m_valuesCapacity * 2;
				if (new_capacity == 0)
					new_capacity = 8;

				if (new_capacity < m_valuesCapacity + i)
					new_capacity = m_valuesCapacity + i;

				valuesGrow(new_capacity);
			}
		}

		template<typename R, typename U>
		void valuesPush(R&& key, U&& value)
		{
			valuesEnsureSpaceExists();
			m_allocator->commit(m_values + m_valuesCount, sizeof(KeyValue<const TKey, TValue>));
			::new (m_values + m_valuesCount) KeyValue<const TKey, TValue>{std::forward<R>(key), std::forward<U>(value)};
			++m_valuesCount;
		}

		void valuesShrinkToFit()
		{
			valuesGrow(m_valuesCount);
		}

	public:
		using iterator = KeyValue<const TKey, TValue>*;

		explicit Map(Allocator* a)
			: m_allocator(a)
		{}

		Map(const Map& other)
		{
			copyFrom(other);
		}

		Map(Map&& other)
		{
			moveFrom(std::move(other));
		}

		Map& operator=(const Map& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		Map& operator=(Map&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~Map()
		{
			destroy();
		}

		template<typename R, typename U>
		void insert(R&& key, U&& value = U{})
		{
			maintainSpaceComplexity();

			auto res = findSlotForInsert(m_slots, m_slotsCount, m_values, key);

			auto& slot = m_slots[res.index];
			auto flags = slot.flags();
			switch (flags)
			{
			case HASH_EMPTY:
			{
				slot.setFlags(HASH_USED);
				slot.setValueIndex(m_valuesCount);
				slot.setValueHash(res.hash);
				valuesPush(std::forward<R>(key), std::forward<U>(value));
				return;
			}
			case HASH_DELETED:
			{
				slot.setFlags(HASH_USED);
				slot.setValueIndex(m_valuesCount);
				slot.setValueHash(res.hash);
				--m_deletedCount;
				valuesPush(std::forward<R>(key), std::forward<U>(value));
			}
			case HASH_USED:
			{
				return;
			}
			default:
			{
				coreUnreachable();
				return;
			}
			}
		}

		void clear()
		{
			for (size_t i = 0; i < m_slotsCount; ++i)
				m_slots[i] = HashSlot{};
			for (size_t i = 0; i < m_valuesCount; ++i)
				m_values[i].~KeyValue();
			m_valuesCount = 0;
			m_deletedCount = 0;
		}

		Allocator* allocator() const { return m_allocator; }

		size_t count() const
		{
			return m_valuesCount;
		}

		size_t capacity() const
		{
			return m_slotsCount;
		}

		void reserve(size_t added_count)
		{
			auto new_cap = m_valuesCount + added_count;
			new_cap *= 4;
			new_cap = new_cap / 3 + 1;
			if (new_cap > m_usedCountThreshold)
			{
				// round up to next power of 2
				if (new_cap > 0)
				{
					--new_cap;
					if constexpr (sizeof(size_t) == 4)
					{
						new_cap |= new_cap >> 1;
						new_cap |= new_cap >> 2;
						new_cap |= new_cap >> 4;
						new_cap |= new_cap >> 8;
						new_cap |= new_cap >> 16;
					}
					else if constexpr (sizeof(size_t) == 8)
					{
						new_cap |= new_cap >> 1;
						new_cap |= new_cap >> 2;
						new_cap |= new_cap >> 4;
						new_cap |= new_cap >> 8;
						new_cap |= new_cap >> 16;
						new_cap |= new_cap >> 32;
					}
					else
					{
						static_assert("unexpected sizeof(size_t)");
					}
					++new_cap;
				}
				reserveExact(new_cap);
			}
		}

		template<typename R>
		const iterator lookup(const R& key) const
		{
			auto res = findSlotForLookup(key);
			if (res.index == m_slotsCount)
				return end();
			auto& slot = m_slots[res.index];
			auto index = slot.valueIndex();
			return m_values + index;
		}

		template<typename R>
		bool remove(const R& key)
		{
			auto res = findSlotForLookup(key);
			if (res.index == m_slotsCount)
				return false;
			auto& slot = m_slots[res.index];
			auto index = slot.valueIndex();
			slot.setFlags(HASH_DELETED);

			m_values[index].~KeyValue();

			if (index < m_valuesCount - 1)
			{
				// fixup the index of the last element after swap
				auto last_res = findSlotForLookup(m_values[m_valuesCount - 1].key);
				m_slots[last_res.index].setValueIndex(index);
				::new (m_values + index) KeyValue<const TKey, TValue>(std::move(m_values[m_valuesCount - 1]));
				m_values[m_valuesCount - 1].~KeyValue();
			}

			--m_valuesCount;
			++m_deletedCount;

			// rehash because of size is too low
			if (m_valuesCount < m_usedCountShrinkThreshold && m_slotsCount > 8)
			{
				reserveExact(m_slotsCount >> 1);
				valuesShrinkToFit();
			}
			// rehash because of too many deleted values
			else if (m_deletedCount > m_deletedCountThreshold)
			{
				reserveExact(m_slotsCount);
			}
			return true;
		}

		iterator begin()
		{
			return m_values;
		}

		const iterator begin() const
		{
			return m_values;
		}

		iterator end()
		{
			return m_values + m_valuesCount;
		}

		const iterator end() const
		{
			return m_values + m_valuesCount;
		}
	};
}

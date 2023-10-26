#pragma once

#include "core/Allocator.h"
#include "core/String.h"
#include "core/Buffer.h"
#include "core/UUID.h"
#include "core/Unique.h"
#include "core/Shared.h"

#include <cstdint>
#include <cstring>
#include <cassert>
#include <new>
#include <utility>

namespace core
{
	template<typename T>
	struct Hash
	{
		inline size_t operator()(const T&) const
		{
			static_assert(sizeof(T) == 0, "Hash not implemented for this type");
			return 0;
		}
	};

	template<typename T>
	struct Hash<T*>
	{
		inline size_t operator()(const T* ptr) const
		{
			return (size_t)ptr;
		}
	};

	template<>
	struct Hash<bool>
	{
		inline size_t operator()(bool value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<char>
	{
		inline size_t operator()(char value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<short>
	{
		inline size_t operator()(short value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<int>
	{
		inline size_t operator()(int value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<long>
	{
		inline size_t operator()(long value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<long long>
	{
		inline size_t operator()(long long value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned char>
	{
		inline size_t operator()(unsigned char value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned short>
	{
		inline size_t operator()(unsigned short value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned int>
	{
		inline size_t operator()(unsigned int value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned long>
	{
		inline size_t operator()(unsigned long value) const
		{
			return size_t(value);
		}
	};

	template<>
	struct Hash<unsigned long long>
	{
		inline size_t operator()(unsigned long long value) const
		{
			return size_t(value);
		}
	};

	template<typename T>
	struct Hash<Unique<T>>
	{
		inline size_t operator()(const Unique<T>& value) const
		{
			return Hash<T*>{}(value.get());
		}
	};

	template<typename T>
	struct Hash<Shared<T>>
	{
		inline size_t operator()(const Shared<T>& value) const
		{
			return Hash<T*>{}(value.get());
		}
	};

	inline static size_t _MurmurHashUnaligned2(const void* ptr, size_t len, size_t seed)
	{
		if constexpr (sizeof(void*) == 4)
		{
			const size_t m = 0x5bd1e995;
			size_t hash = seed ^ len;
			const unsigned char* buffer = static_cast<const unsigned char*>(ptr);

			while (len >= 4)
			{
				size_t k = *reinterpret_cast<const size_t*>(buffer);
				k *= m;
				k ^= k >> 24;
				k *= m;
				hash *= m;
				hash ^= k;
				buffer += 4;
				len -= 4;
			}

			if (len == 3)
			{
				hash ^= static_cast<unsigned char>(buffer[2]) << 16;
				--len;
			}

			if (len == 2)
			{
				hash ^= static_cast<unsigned char>(buffer[1]) << 8;
				--len;
			}

			if (len == 1)
			{
				hash ^= static_cast<unsigned char>(buffer[0]);
				hash *= m;
				--len;
			}

			hash ^= hash >> 13;
			hash *= m;
			hash ^= hash >> 15;
			return hash;
		}
		else if constexpr (sizeof(void*) == 8)
		{
			auto load_bytes = [](const unsigned char* p, intptr_t n) -> size_t
			{
				size_t result = 0;
				--n;
				do
					result = (result << 8) + static_cast<unsigned char>(p[n]);
				while (--n >= 0);
				return result;
			};

			auto shift_mix = [](size_t v) -> size_t
			{
				return v ^ (v >> 47);
			};

			static const size_t mul = (static_cast<size_t>(0xc6a4a793UL) << 32UL) + static_cast<size_t>(0x5bd1e995UL);

			const unsigned char* const buffer = static_cast<const unsigned char*>(ptr);

			const int32_t len_aligned = len & ~0x7;
			const unsigned char* const end = buffer + len_aligned;

			size_t hash = seed ^ (len * mul);
			for (const unsigned char* p = buffer;
				p != end;
				p += 8)
			{
				const size_t data = shift_mix((*reinterpret_cast<const size_t*>(p)) * mul) * mul;
				hash ^= data;
				hash *= mul;
			}

			if ((len & 0x7) != 0)
			{
				const size_t data = load_bytes(end, len & 0x7);
				hash ^= data;
				hash *= mul;
			}

			hash = shift_mix(hash) * mul;
			hash = shift_mix(hash);
			return hash;
		}
	}

	// hashes a block of bytes using murmur hash algorithm
	inline static size_t murmurHash(const void* ptr, size_t len, size_t seed = 0xc70f6907UL)
	{
		return _MurmurHashUnaligned2(ptr, len, seed);
	}

	template<>
	struct Hash<float>
	{
		inline size_t operator()(float value) const
		{
			return value != 0.0f ? murmurHash(&value, sizeof(float)) : 0;
		}
	};

	template<>
	struct Hash<double>
	{
		inline size_t operator()(double value) const
		{
			return value != 0.0f ? murmurHash(&value, sizeof(double)) : 0;
		}
	};

	template<>
	struct Hash<String>
	{
		inline size_t operator()(const String& value) const
		{
			return murmurHash(value.data(), value.count());
		}
	};

	template<>
	struct Hash<StringView>
	{
		inline size_t operator()(StringView value) const
		{
			return murmurHash(value.data(), value.count());
		}
	};

	template<>
	struct Hash<Buffer>
	{
		inline size_t operator()(const Buffer& value) const
		{
			return murmurHash(value.data(), value.count());
		}
	};

	template<>
	struct Hash<UUID>
	{
		inline size_t operator()(UUID value) const
		{
			return murmurHash(&value, sizeof(value));
		}
	};

	// mixes two hash values together
	inline static size_t hashMix(size_t a, size_t b)
	{
		if constexpr (sizeof(size_t) == 4)
		{
			return (b + 0x9e3779b9 + (a << 6) + (a >> 2));
		}
		else if constexpr (sizeof(size_t) == 8)
		{
			a ^= b;
			a *= 0xff51afd7ed558ccd;
			a ^= a >> 32;
			return a;
		}
	}

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
		size_t m_slots_count = 0;
		size_t m_deleted_count = 0;
		size_t m_used_count_threshold = 0;
		size_t m_used_count_shrink_threshold = 0;
		size_t m_deleted_count_threshold = 0;
		T* m_values = nullptr;
		size_t m_values_count = 0;
		size_t m_values_capacity = 0;

		struct Search_Result
		{
			size_t hash;
			size_t index;
		};

		void destroy()
		{
			for (size_t i = 0; i < m_values_count; ++i)
				m_values[i].~T();
			m_allocator->release(m_values, sizeof(T) * m_values_capacity);
			m_allocator->free(m_values, sizeof(T) * m_values_capacity);

			m_allocator->release(m_slots, sizeof(HashSlot) * m_slots_count);
			m_allocator->free(m_slots, sizeof(HashSlot) * m_slots_count);

			m_slots = nullptr;
			m_slots_count = 0;
			m_deleted_count = 0;
			m_used_count_threshold = 0;
			m_used_count_shrink_threshold = 0;
			m_deleted_count_threshold = 0;
			m_values = nullptr;
			m_values_count = 0;
			m_values_capacity = 0;
		}

		void copyFrom(const Set& other)
		{
			m_allocator = other.m_allocator;
			m_slots_count = other.count;
			m_deleted_count = other.m_deleted_count;
			m_used_count_threshold = other.m_used_count_threshold;
			m_used_count_shrink_threshold = other.m_used_count_shrink_threshold;
			m_deleted_count_threshold = other.m_deleted_count_threshold;
			m_values_count = other.m_values_count;
			m_values_capacity = m_values_count;

			m_slots = (HashSlot*)m_allocator->alloc(sizeof(HashSlot) * m_slots_count, alignof(HashSlot));
			m_allocator->commit(m_slots, sizeof(HashSlot) * m_slots_count);
			::memcpy(m_slots, other.m_slots, sizeof(HashSlot) * m_slots_count);

			m_values = (T*)m_allocator->alloc(sizeof(T) * m_values_count, alignof(T));
			m_allocator->commit(m_values, sizeof(T) * m_values_count);
			for (size_t i = 0; i < m_values_count; ++i)
				::new (m_values + i) T(other.m_values[i]);
		}

		void moveFrom(Set&& other)
		{
			m_allocator = other.m_allocator;
			m_slots = other.m_slots;
			m_slots_count = other.m_slots_count;
			m_deleted_count = other.m_deleted_count;
			m_used_count_threshold = other.m_used_count_threshold;
			m_used_count_shrink_threshold = other.m_used_count_shrink_threshold;
			m_deleted_count_threshold = other.m_deleted_count_threshold;
			m_values = other.m_values;
			m_values_count = other.m_values_count;
			m_values_capacity = other.m_values_capacity;

			other.m_slots = nullptr;
			other.m_slots_count = 0;
			other.m_deleted_count = 0;
			other.m_used_count_threshold = 0;
			other.m_used_count_shrink_threshold = 0;
			other.m_deleted_count_threshold = 0;
			other.m_values = nullptr;
			other.m_values_count = 0;
			other.m_values_capacity = 0;
		}

		Search_Result
		findSlotForInsert(const HashSlot* _slots, size_t _slots_count, const T* _values, const T& key, size_t* external_hash)
		{
			Search_Result res{};
			if (external_hash)
				res.hash = *external_hash;
			else
				res.hash = THash{}(key);

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
					assert(false);
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
			res.hash = THash{}(key);

			auto cap = m_slots_count;
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

			m_deleted_count = 0;
			// if 12/16th of table is occupied, grow
			m_used_count_threshold = new_count - (new_count >> 2);
			// if deleted count is 3/16th of table, rebuild
			m_deleted_count_threshold = (new_count >> 3) + (new_count >> 4);
			// if table is only 4/16th full, shrink
			m_used_count_shrink_threshold = new_count >> 2;

			// do a rehash
			if (m_values_count != 0)
			{
				for (size_t i = 0; i < m_slots_count; ++i)
				{
					const auto& slot = m_slots[i];
					auto flags = slot.flags();
					if (flags == HASH_USED)
					{
						auto index = slot.valueIndex();
						auto hash = slot.valueHash();
						auto res = findSlotForInsert(new_slots, new_count, m_values, m_values[index], &hash);
						new_slots[res.index] = slot;
					}
				}
			}

			m_allocator->release(m_slots, sizeof(HashSlot) * m_slots_count);
			m_allocator->free(m_slots, sizeof(HashSlot) * m_slots_count);
			m_slots = new_slots;
			m_slots_count = new_count;
		}

		void maintainSpaceComplexity()
		{
			if (m_slots_count == 0)
			{
				reserveExact(8);
			}
			else if (m_values_count + 1 > m_used_count_threshold)
			{
				reserveExact(m_slots_count * 2);
			}
		}

		void valuesGrow(size_t new_capacity)
		{
			auto new_ptr = (T*)m_allocator->alloc(sizeof(T) * new_capacity, alignof(T));
			m_allocator->commit(new_ptr, sizeof(T) * m_values_count);
			for (size_t i = 0; i < m_values_count; ++i)
			{
				if constexpr (std::is_move_constructible_v<T>)
					::new (new_ptr + i) T(std::move(m_values[i]));
				else
					::new (new_ptr + i) T(m_values[i]);
				m_values[i].~T();
			}

			m_allocator->release(m_values, sizeof(T) * m_values_capacity);
			m_allocator->free(m_values, sizeof(T) * m_values_capacity);

			m_values = new_ptr;
			m_values_capacity = new_capacity;
		}

		void valuesEnsureSpaceExists(size_t i = 1)
		{
			if (m_values_count + i > m_values_capacity)
			{
				size_t new_capacity = m_values_capacity * 2;
				if (new_capacity == 0)
					new_capacity = 8;

				if (new_capacity < m_values_capacity + i)
					new_capacity = m_values_capacity + i;

				valuesGrow(new_capacity);
			}
		}

		void valuesPush(const T& key)
		{
			valuesEnsureSpaceExists();
			m_allocator->commit(m_values + m_values_count, sizeof(T));
			::new (m_values + m_values_count) T(std::move(key));
			++m_values_count;
		}

		void valuesShrinkToFit()
		{
			valuesGrow(m_values_count);
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
		void insert(const R& key)
		{
			maintainSpaceComplexity();

			auto res = findSlotForInsert(m_slots, m_slots_count, m_values, key, nullptr);

			auto& slot = m_slots[res.index];
			auto flags = slot.flags();
			switch (flags)
			{
			case HASH_EMPTY:
			{
				slot.setFlags(HASH_USED);
				slot.setValueIndex(m_values_count);
				slot.setValueHash(res.hash);
				valuesPush(key);
				return;
			}
			case HASH_DELETED:
			{
				slot.setFlags(HASH_USED);
				slot.setValueIndex(m_values_count);
				slot.setValueHash(res.hash);
				--m_deleted_count;
				valuesPush(key);
			}
			case HASH_USED:
			{
				return;
			}
			default:
			{
				assert(false);
				return;
			}
			}
		}

		void clear()
		{
			for (size_t i = 0; i < m_slots_count; ++i)
				m_slots[i] = HashSlot{};
			for (size_t i = 0; i < m_values_count; ++i)
				m_values[i].~T();
			m_values_count = 0;
			m_deleted_count = 0;
		}

		Allocator* allocator() const { return m_allocator; }

		size_t count() const
		{
			return m_values_count;
		}

		size_t capacity() const
		{
			return m_slots_count;
		}

		void reserve(size_t added_count)
		{
			auto new_cap = m_values_count + added_count;
			new_cap *= 4;
			new_cap = new_cap / 3 + 1;
			if (new_cap > m_used_count_threshold)
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
			if (res.index == m_slots_count)
				return nullptr;
			auto& slot = m_slots[res.index];
			auto index = slot.valueIndex();
			return m_values + index;
		}

		template<typename R>
		bool remove(const R& key)
		{
			auto res = findSlotForLookup(key);
			if (res.index == m_slots_count)
				return false;
			auto& slot = m_slots[res.index];
			auto index = slot.valueIndex();
			slot.setFlags(HASH_DELETED);

			m_values[index].~T();

			if (index < m_values_count - 1)
			{
				// fixup the index of the last element after swap
				auto last_res = findSlotForLookup(m_values[m_values_count - 1]);
				m_slots[last_res.index].setValueIndex(index);
				::new (m_values + index) T(std::move(m_values[m_values_count - 1]));
				m_values[m_values_count - 1].~T();
			}

			--m_values_count;
			++m_deleted_count;

			// rehash because of size is too low
			if (m_values_count < m_used_count_shrink_threshold && m_slots_count > 8)
			{
				reserveExact(m_slots_count >> 1);
				valuesShrinkToFit();
			}
			// rehash because of too many deleted values
			else if (m_deleted_count > m_deleted_count_threshold)
			{
				reserveExact(m_slots_count);
			}
			return true;
		}

		const iterator begin() const
		{
			return m_values;
		}

		const iterator end() const
		{
			return m_values + m_values_count;
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
		size_t m_slots_count = 0;
		size_t m_deleted_count = 0;
		size_t m_used_count_threshold = 0;
		size_t m_used_count_shrink_threshold = 0;
		size_t m_deleted_count_threshold = 0;
		KeyValue<const TKey, TValue>* m_values = nullptr;
		size_t m_values_count = 0;
		size_t m_values_capacity = 0;

		struct Search_Result
		{
			size_t hash;
			size_t index;
		};

		void destroy()
		{
			for (size_t i = 0; i < m_values_count; ++i)
				m_values[i].~KeyValue();
			m_allocator->release(m_values, sizeof(KeyValue<TKey, TValue>) * m_values_capacity);
			m_allocator->free(m_values, sizeof(KeyValue<TKey, TValue>) * m_values_capacity);

			m_allocator->release(m_slots, sizeof(HashSlot) * m_slots_count);
			m_allocator->free(m_slots, sizeof(HashSlot) * m_slots_count);

			m_slots = nullptr;
			m_slots_count = 0;
			m_deleted_count = 0;
			m_used_count_threshold = 0;
			m_used_count_shrink_threshold = 0;
			m_deleted_count_threshold = 0;
			m_values = nullptr;
			m_values_count = 0;
			m_values_capacity = 0;
		}

		void copyFrom(const Map& other)
		{
			m_allocator = other.m_allocator;
			m_slots_count = other.m_slots_count;
			m_deleted_count = other.m_deleted_count;
			m_used_count_threshold = other.m_used_count_threshold;
			m_used_count_shrink_threshold = other.m_used_count_shrink_threshold;
			m_deleted_count_threshold = other.m_deleted_count_threshold;
			m_values_count = other.m_values_count;
			m_values_capacity = m_values_count;

			m_slots = (HashSlot*)m_allocator->alloc(sizeof(HashSlot) * m_slots_count, alignof(HashSlot));
			m_allocator->commit(m_slots, sizeof(HashSlot) * m_slots_count);
			::memcpy(m_slots, other.m_slots, sizeof(HashSlot) * m_slots_count);

			m_values = (KeyValue<const TKey, TValue>*)m_allocator->alloc(sizeof(KeyValue<const TKey, TValue>) * m_values_count, alignof(KeyValue<const TKey, TValue>));
			m_allocator->commit(m_values, sizeof(TValue) * m_values_count);
			for (size_t i = 0; i < m_values_count; ++i)
				::new (m_values + i) KeyValue<const TKey, TValue>(other.m_values[i]);
		}

		void moveFrom(Map&& other)
		{
			m_allocator = other.m_allocator;
			m_slots = other.m_slots;
			m_slots_count = other.m_slots_count;
			m_deleted_count = other.m_deleted_count;
			m_used_count_threshold = other.m_used_count_threshold;
			m_used_count_shrink_threshold = other.m_used_count_shrink_threshold;
			m_deleted_count_threshold = other.m_deleted_count_threshold;
			m_values = other.m_values;
			m_values_count = other.m_values_count;
			m_values_capacity = other.m_values_capacity;

			other.m_slots = nullptr;
			other.m_slots_count = 0;
			other.m_deleted_count = 0;
			other.m_used_count_threshold = 0;
			other.m_used_count_shrink_threshold = 0;
			other.m_deleted_count_threshold = 0;
			other.m_values = nullptr;
			other.m_values_count = 0;
			other.m_values_capacity = 0;
		}

		Search_Result
		findSlotForInsert(const HashSlot* _slots, size_t _slots_count, const KeyValue<const TKey, TValue>* _values, const TKey& key, size_t* external_hash)
		{
			Search_Result res{};
			if (external_hash)
				res.hash = *external_hash;
			else
				res.hash = THash{}(key);

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
					assert(false);
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
			res.hash = THash{}(key);

			auto cap = m_slots_count;
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

			m_deleted_count = 0;
			// if 12/16th of table is occupied, grow
			m_used_count_threshold = new_count - (new_count >> 2);
			// if deleted count is 3/16th of table, rebuild
			m_deleted_count_threshold = (new_count >> 3) + (new_count >> 4);
			// if table is only 4/16th full, shrink
			m_used_count_shrink_threshold = new_count >> 2;

			// do a rehash
			if (m_values_count != 0)
			{
				for (size_t i = 0; i < m_slots_count; ++i)
				{
					const auto& slot = m_slots[i];
					auto flags = slot.flags();
					if (flags == HASH_USED)
					{
						auto index = slot.valueIndex();
						auto hash = slot.valueHash();
						auto res = findSlotForInsert(new_slots, new_count, m_values, m_values[index].key, &hash);
						new_slots[res.index] = slot;
					}
				}
			}

			m_allocator->release(m_slots, sizeof(HashSlot) * m_slots_count);
			m_allocator->free(m_slots, sizeof(HashSlot) * m_slots_count);
			m_slots = new_slots;
			m_slots_count = new_count;
		}

		void maintainSpaceComplexity()
		{
			if (m_slots_count == 0)
			{
				reserveExact(8);
			}
			else if (m_values_count + 1 > m_used_count_threshold)
			{
				reserveExact(m_slots_count * 2);
			}
		}

		void valuesGrow(size_t new_capacity)
		{
			auto new_ptr = (KeyValue<const TKey, TValue>*)m_allocator->alloc(sizeof(KeyValue<const TKey, TValue>) * new_capacity, alignof(KeyValue<const TKey, TValue>));
			m_allocator->commit(new_ptr, sizeof(*m_values) * m_values_count);
			for (size_t i = 0; i < m_values_count; ++i)
			{
				if constexpr (std::is_move_constructible_v<KeyValue<const TKey, TValue>>)
					::new (new_ptr + i) KeyValue<const TKey, TValue>(std::move(m_values[i]));
				else
					::new (new_ptr + i) KeyValue<const TKey, TValue>(m_values[i]);
				m_values[i].~KeyValue();
			}

			m_allocator->release(m_values, sizeof(*m_values) * m_values_capacity);
			m_allocator->free(m_values, sizeof(*m_values) * m_values_capacity);

			m_values = new_ptr;
			m_values_capacity = new_capacity;
		}

		void valuesEnsureSpaceExists(size_t i = 1)
		{
			if (m_values_count + i > m_values_capacity)
			{
				size_t new_capacity = m_values_capacity * 2;
				if (new_capacity == 0)
					new_capacity = 8;

				if (new_capacity < m_values_capacity + i)
					new_capacity = m_values_capacity + i;

				valuesGrow(new_capacity);
			}
		}

		void valuesPush(const KeyValue<const TKey, TValue>& key)
		{
			valuesEnsureSpaceExists();
			m_allocator->commit(m_values + m_values_count, sizeof(KeyValue<const TKey, TValue>));
			::new (m_values + m_values_count) KeyValue<const TKey, TValue>(std::move(key));
			++m_values_count;
		}

		void valuesShrinkToFit()
		{
			valuesGrow(m_values_count);
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
		void insert(const R& key, const U& value = U{})
		{
			maintainSpaceComplexity();

			auto res = findSlotForInsert(m_slots, m_slots_count, m_values, key, nullptr);

			auto& slot = m_slots[res.index];
			auto flags = slot.flags();
			switch (flags)
			{
			case HASH_EMPTY:
			{
				slot.setFlags(HASH_USED);
				slot.setValueIndex(m_values_count);
				slot.setValueHash(res.hash);
				valuesPush(KeyValue<const TKey, TValue>{key, value});
				return;
			}
			case HASH_DELETED:
			{
				slot.setFlags(HASH_USED);
				slot.setValueIndex(m_values_count);
				slot.setValueHash(res.hash);
				--m_deleted_count;
				valuesPush(KeyValue<const TKey, TValue>{key, value});
			}
			case HASH_USED:
			{
				return;
			}
			default:
			{
				assert(false);
				return;
			}
			}
		}

		void clear()
		{
			for (size_t i = 0; i < m_slots_count; ++i)
				m_slots[i] = HashSlot{};
			for (size_t i = 0; i < m_values_count; ++i)
				m_values[i].~KeyValue();
			m_values_count = 0;
			m_deleted_count = 0;
		}

		Allocator* allocator() const { return m_allocator; }

		size_t count() const
		{
			return m_values_count;
		}

		size_t capacity() const
		{
			return m_slots_count;
		}

		void reserve(size_t added_count)
		{
			auto new_cap = m_values_count + added_count;
			new_cap *= 4;
			new_cap = new_cap / 3 + 1;
			if (new_cap > m_used_count_threshold)
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
			if (res.index == m_slots_count)
				return end();
			auto& slot = m_slots[res.index];
			auto index = slot.valueIndex();
			return m_values + index;
		}

		template<typename R>
		bool remove(const R& key)
		{
			auto res = findSlotForLookup(key);
			if (res.index == m_slots_count)
				return false;
			auto& slot = m_slots[res.index];
			auto index = slot.valueIndex();
			slot.setFlags(HASH_DELETED);

			m_values[index].~KeyValue();

			if (index < m_values_count - 1)
			{
				// fixup the index of the last element after swap
				auto last_res = findSlotForLookup(m_values[m_values_count - 1].key);
				m_slots[last_res.index].setValueIndex(index);
				::new (m_values + index) KeyValue<const TKey, TValue>(std::move(m_values[m_values_count - 1]));
				m_values[m_values_count - 1].~KeyValue();
			}

			--m_values_count;
			++m_deleted_count;

			// rehash because of size is too low
			if (m_values_count < m_used_count_shrink_threshold && m_slots_count > 8)
			{
				reserveExact(m_slots_count >> 1);
				valuesShrinkToFit();
			}
			// rehash because of too many deleted values
			else if (m_deleted_count > m_deleted_count_threshold)
			{
				reserveExact(m_slots_count);
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
			return m_values + m_values_count;
		}

		const iterator end() const
		{
			return m_values + m_values_count;
		}
	};
}

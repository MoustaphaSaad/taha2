#pragma once

#include "core/Allocator.h"
#include "core/Assert.h"

#include <new>
#include <utility>

namespace core
{
	template<typename T>
	class Queue
	{
		struct Node
		{
			Node* next = nullptr;
			Node* prev = nullptr;
			T value;
		};

		Allocator* m_allocator = nullptr;
		Node* m_head = nullptr;
		Node* m_tail = nullptr;
		size_t m_count = 0;

		void destroy()
		{
			auto node = m_head;
			while (node)
			{
				auto next = node->next;
				node->~Node();
				m_allocator->releaseSingleT(node);
				m_allocator->freeSingleT(node);
				node = next;
			}
		}

		void copyFrom(const Queue& other)
		{
			m_allocator = other.m_allocator;
			m_count = other.m_count;
			auto node = other.m_head;
			while (node)
			{
				auto new_node = m_allocator->allocSingleT<Node>();
				m_allocator->commitSingleT(new_node);
				new (new_node) Node{ nullptr, nullptr, node->value };
				if (m_tail)
				{
					m_tail->next = new_node;
					new_node->prev = m_tail;
					m_tail = new_node;
				}
				else
				{
					m_head = new_node;
					m_tail = new_node;
				}
				node = node->next;
			}
		}

		void moveFrom(Queue& other)
		{
			m_allocator = other.m_allocator;
			m_head = other.m_head;
			m_tail = other.m_tail;
			m_count = other.m_count;

			other.m_allocator = nullptr;
			other.m_head = nullptr;
			other.m_tail = nullptr;
			other.m_count = 0;
		}
	public:
		explicit Queue(Allocator* allocator)
			: m_allocator(allocator)
		{}

		Queue(const Queue& other)
		{
			copyFrom(other);
		}

		Queue(Queue&& other) noexcept
		{
			moveFrom(other);
		}

		Queue& operator=(const Queue& other)
		{
			destroy();
			copyFrom(other);
			return *this;
		}

		Queue& operator=(Queue&& other) noexcept
		{
			destroy();
			moveFrom(other);
			return *this;
		}

		~Queue()
		{
			destroy();
		}

		template<typename R>
		void push_back(R&& value)
		{
			auto node = m_allocator->allocSingleT<Node>();
			m_allocator->commitSingleT(node);
			new (node) Node{ nullptr, nullptr, std::forward<R>(value) };
			if (m_tail)
			{
				m_tail->next = node;
				node->prev = m_tail;
				m_tail = node;
			}
			else
			{
				m_head = node;
				m_tail = node;
			}
			++m_count;
		}

		template<typename R>
		void push_front(R&& value)
		{
			auto node = m_allocator->allocSingleT<Node>();
			m_allocator->commitSingleT(node);
			new (node) Node{ nullptr, nullptr, std::forward<R>(value) };
			if (m_head)
			{
				m_head->prev = node;
				node->next = m_head;
				m_head = node;
			}
			else
			{
				m_head = node;
				m_tail = node;
			}
			++m_count;
		}

		T& back()
		{
			coreAssert(m_tail);
			return m_tail->value;
		}

		const T& back() const
		{
			coreAssert(m_tail);
			return m_tail->value;
		}

		T& front()
		{
			coreAssert(m_head);
			return m_head->value;
		}

		const T& front() const
		{
			coreAssert(m_head);
			return m_head->value;
		}

		void pop_back()
		{
			coreAssert(m_tail);
			auto node = m_tail;
			m_tail = m_tail->prev;
			if (m_tail)
				m_tail->next = nullptr;
			else
				m_head = nullptr;
			node->~Node();
			m_allocator->releaseSingleT(node);
			m_allocator->freeSingleT(node);
			--m_count;
		}

		void pop_front()
		{
			coreAssert(m_head);
			auto node = m_head;
			m_head = m_head->next;
			if (m_head)
				m_head->prev = nullptr;
			else
				m_tail = nullptr;
			node->~Node();
			m_allocator->releaseSingleT(node);
			m_allocator->freeSingleT(node);
			--m_count;
		}

		size_t count() const { return m_count; }
		Allocator* allocator() const { return m_allocator; }
	};
}
#pragma once

#include "core/Unique.h"

#include <functional>

namespace core
{
	template<typename>
	class Func;

	template<typename TReturn, typename ... TArgs>
	class Func<TReturn(TArgs...)>
	{
		struct Concept
		{
			void (*dtor)(void*) noexcept;
			void (*move)(void*, void*) noexcept;
			TReturn (*invoke)(void*, TArgs&& ...);
		};

		static constexpr size_t SMALL_SIZE = sizeof(void*) * 4;
		static constexpr Concept EMPTY_CONCEPT{[](void*) noexcept {}};
		alignas(Concept) std::byte m_model[SMALL_SIZE];
		const Concept* m_concept = &EMPTY_CONCEPT;

		template<typename TFunc, bool IsSmall>
		struct Model;

		template<typename TFunc>
		struct Model<TFunc, true>
		{
			TFunc m_func;

			template<typename F>
			Model(Allocator*, F&& f)
				: m_func(std::forward<F>(f))
			{}

			static void dtor(void* self) noexcept { static_cast<Model*>(self)->~Model(); }
			static void move(void* self, void* p) noexcept
			{
				new (p) Model(std::move(*static_cast<Model*>(self)));
			}
			static TReturn invoke(void* self, TArgs&& ... args)
			{
				return std::invoke(static_cast<Model*>(self)->m_func, std::forward<TArgs>(args)...);
			}

			static constexpr Concept vtable{dtor, move, invoke};
		};

		template<typename TFunc>
		struct Model<TFunc, false>
		{
			Unique<TFunc> m_func;

			template<typename F>
			Model(Allocator* allocator, F&& f)
				: m_func(unique_from<TFunc>(allocator, std::forward<F>(f)))
			{}

			static void dtor(void* self) noexcept { static_cast<Model*>(self)->~Model(); }
			static void move(void* self, void* p) noexcept
			{
				new (p) Model(std::move(*static_cast<Model*>(self)));
			}
			static TReturn invoke(void* self, TArgs&& ... args)
			{
				return std::invoke(*static_cast<Model*>(self)->m_func, std::forward<TArgs>(args)...);
			}

			static constexpr Concept vtable{dtor, move, invoke};
		};

	public:
		Func() = default;

		template<typename F>
		Func(F&& f)
		{
			constexpr bool is_small = sizeof(Model<std::decay_t<F>, true>) <= SMALL_SIZE;
			static_assert(is_small);
			new (&m_model) Model<std::decay_t<F>, is_small>(nullptr, std::forward<F>(f));
			m_concept = &Model<std::decay_t<F>, is_small>::vtable;
		}

		template<typename F>
		Func(Allocator* allocator, F&& f)
		{
			constexpr bool is_small = sizeof(Model<std::decay_t<F>, true>) <= SMALL_SIZE;
			new (&m_model) Model<std::decay_t<F>, is_small>(allocator, std::forward<F>(f));
			m_concept = &Model<std::decay_t<F>, is_small>::vtable;
		}

		Func(Func&& other) noexcept
			: m_concept(other.m_concept)
		{
			m_concept->move(&other.m_model, &m_model);
			other.m_concept = &EMPTY_CONCEPT;
		}

		Func& operator=(Func&& other) noexcept
		{
			m_concept->dtor(&m_model);
			m_concept = other.m_concept;
			m_concept->move(&other.m_model, &m_model);
			other.m_concept = &EMPTY_CONCEPT;
			return *this;
		}

		~Func() { m_concept->dtor(&m_model); }

		TReturn operator()(TArgs ... args)
		{
			return m_concept->invoke(&m_model, std::forward<TArgs>(args)...);
		}

		operator bool() const { return m_concept != &EMPTY_CONCEPT; }
	};
}
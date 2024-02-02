#pragma once

#include "core/Exports.h"
#include "core/Result.h"
#include "core/Unique.h"
#include "core/ThreadPool.h"
#include "core/Log.h"
#include "core/Allocator.h"
#include "core/Hash.h"

namespace core
{
	class ReactorLoop;

	class ReactorEvent
	{
	public:
		virtual ~ReactorEvent() = default;
	};

	template<typename T>
	inline constexpr auto ReactorEventID = typeid(T).hash_code();

	class Reactor
	{
		ReactorLoop* m_loop = nullptr;
	public:
		explicit Reactor(ReactorLoop* loop)
			: m_loop(loop)
		{}

		virtual ~Reactor() = default;

		virtual void handle(ReactorEvent* event) = 0;

		ReactorLoop* reactorLoop() const { return m_loop; }
	};

	class ReactorLoop
	{
	public:
		CORE_EXPORT static Result<Unique<ReactorLoop>> create(ThreadPool* pool, Log* log, Allocator* allocator);

		virtual ~ReactorLoop() = default;
	};
}
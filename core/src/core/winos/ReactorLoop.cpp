#include "core/ReactorLoop.h"

namespace core
{
	class WinOSReactorLoop: public ReactorLoop
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
		ThreadPool* m_pool = nullptr;
	public:
		WinOSReactorLoop(ThreadPool* pool, Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log),
			  m_pool(pool)
		{}
	};

	Result<Unique<ReactorLoop>> ReactorLoop::create(ThreadPool *pool, Log *log, Allocator *allocator)
	{
		auto res = unique_from<WinOSReactorLoop>(allocator, pool, log, allocator);
		return res;
	}
}
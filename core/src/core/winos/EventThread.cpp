#include "core/EventThread.h"

namespace core
{
	class WinOSThreadPool: public EventThreadPool
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
	public:
		WinOSThreadPool(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log)
		{}
	};

	Result<Unique<EventThreadPool>> EventThreadPool::create(Log *log, Allocator *allocator)
	{
		auto res = unique_from<WinOSThreadPool>(allocator, log, allocator);
		return res;
	}
}
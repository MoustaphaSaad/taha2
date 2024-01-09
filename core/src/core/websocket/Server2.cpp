#include "core/websocket/Server2.h"

namespace core::websocket
{
	class Server2Impl: public Server2
	{
		Allocator* m_allocator = nullptr;
		Log* m_log = nullptr;
	public:
		Server2Impl(Log* log, Allocator* allocator)
			: m_allocator(allocator),
			  m_log(log)
		{}
	};

	Result<Unique<Server2>> Server2::create(Log *log, Allocator *allocator)
	{
		return unique_from<Server2Impl>(allocator, log, allocator);
	}
}
#include "core/websocket/Client2.h"
#include "core/websocket/Internal.h"

namespace core::websocket
{
	Result<Unique<Client2>> Client2::connect(const Client2Config& config, EventLoop2* loop, Log *log, Allocator *allocator)
	{
		return ImplClient2::connect(config, loop, log, allocator);
	}
}
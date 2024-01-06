#include "core/EventLoop.h"

#include <Windows.h>

namespace core
{
	class WinOSEventLoop: public EventLoop
	{
		Allocator* m_allocator = nullptr;
		HANDLE m_completionPort = INVALID_HANDLE_VALUE;
	public:
		WinOSEventLoop(HANDLE completionPort, Allocator* allocator)
			: m_allocator(allocator),
			  m_completionPort(completionPort)
		{}

		HumanError run() override
		{
			// TODO: implement function
			return {};
		}

		void stop() override
		{
			// TODO: implement function
		}
	};

	Result<Unique<EventLoop>> EventLoop::create(Allocator* allocator)
	{
		auto completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (completionPort == NULL)
			return errf(allocator, "failed to create completion port"_sv);

		return unique_from<WinOSEventLoop>(allocator, completionPort, allocator);
	}
}
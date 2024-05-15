#include "core/EventLoop.h"
#include "core/Assert.h"

namespace core
{
	class MacEventLoop: public EventLoop
	{
	public:
		HumanError run() override
		{
			coreUnreachable();
			return {};
		}

		void stop() override
		{
			coreUnreachable();
		}

		void stopAllLoops() override
		{
			coreUnreachable();
		}

		Result<EventSocket> registerSocket(Unique<Socket> socket) override
		{
			coreUnreachable();
			return HumanError{};
		}

		EventLoop* next() override
		{
			coreUnreachable();
			return nullptr;
		}
	};

	Result<Unique<EventLoop>> EventLoop::create(ThreadedEventLoop* parent, Log* log, Allocator* allocator)
	{
		coreUnreachable();
		return errf(allocator, "not implemented"_sv);
	}

	HumanError EventThread::send(Unique<Event> event)
	{
		coreUnreachable();
		return {};
	}

	HumanError EventThread::stop()
	{
		coreUnreachable();
		return {};
	}

	bool EventThread::stopped() const
	{
		coreUnreachable();
		return false;
	}
}
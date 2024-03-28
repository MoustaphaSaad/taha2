#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Thread.h>
#include <core/Chan.h>

void evenIntegerProducer(core::Shared<core::Chan<int>> out, core::Log* log)
{
	for (int i = 2; i <= 100; i += 2)
	{
		log->debug("sending {}"_sv, i);
		out->send(&i);
		log->debug("sent {}"_sv, i);
	}
	out->close();
}

void oddIntegerProducer(core::Shared<core::Chan<int>> out, core::Log* log)
{
	for (int i = 1; i <= 100; i += 2)
	{
		log->debug("sending {}"_sv, i);
		out->send(&i);
		log->debug("sent {}"_sv, i);
	}
	out->close();
}

void consumer(core::Shared<core::Chan<int>> evenNumbers, core::Shared<core::Chan<int>> oddNumbers, core::Log* log, core::Allocator* allocator)
{
	while (evenNumbers->isClosed() == false || oddNumbers->isClosed() == false)
	{
		core::Select
		{
		core::ReadCase{allocator, *evenNumbers} = [&](int num)
		{
			log->info("even consumer: {}"_sv, num);
		},
		core::DefaultCase{allocator} = [&]
		{
			log->info("default"_sv);
		},
		core::ReadCase{allocator, *oddNumbers} = [&](int num)
		{
			log->info("odd consumer: {}"_sv, num);
		},
		};
	}
}

int main()
{
	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto evenNumbers = core::Chan<int>::create(0, &allocator);
	core::Thread evenNumberProducerThread{&allocator, [evenNumbers, &log]{ evenIntegerProducer(evenNumbers, &log); }};

	auto oddNumbers = core::Chan<int>::create(0, &allocator);
	core::Thread oddNumberProducerThread{&allocator, [oddNumbers, &log]{ oddIntegerProducer(oddNumbers, &log); }};

	core::Thread consumerThread{&allocator, core::Func<void()>{&allocator, [evenNumbers, oddNumbers, &log, &allocator]{ consumer(evenNumbers, oddNumbers, &log, &allocator); }}};

	oddNumberProducerThread.join();
	evenNumberProducerThread.join();
	consumerThread.join();

	return EXIT_SUCCESS;
}
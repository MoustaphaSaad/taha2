#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Thread.h>
#include <core/Chan.h>

void evenIntegerProducer(core::Shared<core::Chan<int>> out, core::Log *log)
{
	for (int i = 2; i <= 100; i += 2)
	{
		log->debug("sending {}"_sv, i);
		out->send(&i);
		log->debug("sent {}"_sv, i);
	}
	out->close();
}

void oddIntegerProducer(core::Shared<core::Chan<int>> out, core::Log *log)
{
	for (int i = 1; i <= 100; i += 2)
	{
		log->debug("sending {}"_sv, i);
		out->send(&i);
		log->debug("sent {}"_sv, i);
	}
	out->close();
}

void slowEvenIntegerProducer(core::Shared<core::Chan<int>> out, core::Log *log, std::chrono::milliseconds waitTime)
{
	for (int i = 2; i <= 100; i += 2)
	{
		out->send(&i);
		std::this_thread::sleep_for(waitTime);
	}
	out->close();
}

namespace two_producers
{
	void
	consumerDefault(core::Shared<core::Chan<int>> evenNumbers, core::Shared<core::Chan<int>> oddNumbers, core::Log *log,
					core::Allocator *allocator)
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

	void consumer(core::Shared<core::Chan<int>> evenNumbers, core::Shared<core::Chan<int>> oddNumbers, core::Log *log,
				  core::Allocator *allocator)
	{
		while (evenNumbers->isClosed() == false || oddNumbers->isClosed() == false)
		{
			core::Select
			{
			core::ReadCase{allocator, *evenNumbers} = [&](int num)
			{
				log->info("even consumer: {}"_sv, num);
			},
			core::ReadCase{allocator, *oddNumbers} = [&](int num)
			{
				log->info("odd consumer: {}"_sv, num);
			},
			};
		}
	}

	void test()
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
		log.info("two producers: success"_sv);
	}
}

namespace two_producers_fast_slow
{
	void consumer(core::Shared<core::Chan<int>> evenNumbers, core::Shared<core::Chan<int>> oddNumbers, core::Log *log,
				  core::Allocator *allocator)
	{
		while (evenNumbers->isClosed() == false || oddNumbers->isClosed() == false)
		{
			core::Select
			{
				core::ReadCase{allocator, *evenNumbers} = [&](int num)
				{
					log->info("even consumer: {}"_sv, num);
				},
				core::ReadCase{allocator, *oddNumbers} = [&](int num)
				{
					log->info("odd consumer: {}"_sv, num);
				},
				};
		}
	}

	void test()
	{
		core::FastLeak allocator{};
		core::Log log{&allocator};

		auto evenNumbers = core::Chan<int>::create(0, &allocator);
		core::Thread evenNumberProducerThread{&allocator, [evenNumbers, &log]{ slowEvenIntegerProducer(evenNumbers, &log, std::chrono::milliseconds{100}); }};

		auto oddNumbers = core::Chan<int>::create(0, &allocator);
		core::Thread oddNumberProducerThread{&allocator, [oddNumbers, &log]{ oddIntegerProducer(oddNumbers, &log); }};

		core::Thread consumerThread{&allocator, core::Func<void()>{&allocator, [evenNumbers, oddNumbers, &log, &allocator]{ consumer(evenNumbers, oddNumbers, &log, &allocator); }}};

		oddNumberProducerThread.join();
		evenNumberProducerThread.join();
		consumerThread.join();
		log.info("two producers: success"_sv);
	}
}


namespace timeout
{
	void signalTimeout(core::Shared<core::Chan<bool>> out)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{100});
		bool v = true;
		out->send(&v);
		out->close();
	}

	void consumer(core::Shared<core::Chan<int>> numbers, core::Shared<core::Chan<bool>> stop, core::Log* log, core::Allocator* allocator)
	{
		core::Select
		{
		core::ReadCase{allocator, *numbers} = [&](int num)
		{
			log->info("number: {}"_sv, num);
		},
		core::ReadCase{allocator, *stop} = [&](bool stop)
		{
			log->debug("stop"_sv);
		},
		};
	}

	void test()
	{
		core::FastLeak allocator{};
		core::Log log{&allocator};

		auto evenNumbers = core::Chan<int>::create(0, &allocator);
		core::Thread evenNumberProducerThread{&allocator, [evenNumbers, &log]{ slowEvenIntegerProducer(evenNumbers, &log, std::chrono::milliseconds{15}); }};

		auto stop = core::Chan<bool>::create(0, &allocator);
		core::Thread stopThread{&allocator, [stop, &log]{ signalTimeout(stop); }};

		core::Thread consumerThread{&allocator, core::Func<void()>{&allocator, [evenNumbers, stop, &log, &allocator]{ consumer(evenNumbers, stop, &log, &allocator); }}};

		consumerThread.join();
		stopThread.detach();
		evenNumberProducerThread.detach();
		log.info("timeout: success"_sv);
	}
}

namespace load_balancer
{
	void consumer(core::Shared<core::Chan<int>> numbers, core::Log* log, core::Allocator* allocator)
	{
		for (auto num: *numbers)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds{100});
			log->info("number: {}"_sv, num);
		}
	}

	void balanceLoad(core::Shared<core::Chan<int>> tasks, core::Shared<core::Chan<int>> worker1, core::Shared<core::Chan<int>> worker2, core::Allocator* allocator)
	{
		for (auto task: *tasks)
		{
			core::Select
			{
			core::WriteCase{allocator, *worker1, task} = []{},
			core::WriteCase{allocator, *worker2, task} = []{},
			};
		}
		worker1->close();
		worker2->close();
	}

	void test()
	{
		core::FastLeak allocator{};
		core::Log log{&allocator};

		auto evenNumbers = core::Chan<int>::create(0, &allocator);
		core::Thread evenNumberProducerThread{&allocator, [evenNumbers, &log]{ evenIntegerProducer(evenNumbers, &log); }};

		auto worker1Chan = core::Chan<int>::create(0, &allocator);
		core::Thread worker1{&allocator, [worker1Chan, &log, &allocator]{ consumer(worker1Chan, &log, &allocator); }};

		auto worker2Chan = core::Chan<int>::create(0, &allocator);
		core::Thread worker2{&allocator, [worker2Chan, &log, &allocator]{ consumer(worker2Chan, &log, &allocator); }};

		core::Thread loadBalancer{&allocator, core::Func<void()>{&allocator, [evenNumbers, worker1Chan, worker2Chan, &allocator]{ balanceLoad(evenNumbers, worker1Chan, worker2Chan, &allocator); }}};

		evenNumberProducerThread.join();
		loadBalancer.join();
		worker1.join();
		worker2.join();
		log.info("load balancer: success"_sv);
	}
}

int main()
{
	two_producers::test();
	two_producers_fast_slow::test();
	timeout::test();
	load_balancer::test();
	return EXIT_SUCCESS;
}
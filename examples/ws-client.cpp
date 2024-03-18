// command to run the wstest
// docker run -it --rm
// -v "W:\Projects\taha2\test\autobahn-testsuite\config:/config"
// -v "W:\Projects\taha2\test\autobahn-testsuite\reports:/reports"
// -p 9011:9011 --name wstest crossbario/autobahn-testsuite wstest -m fuzzingserver -s /config/fuzzingserver.json

#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/ThreadedEventLoop.h>
#include <core/websocket/Client3.h>
#include <core/Unique.h>

#include <tracy/Tracy.hpp>

#include <signal.h>
#include "core/websocket/Server3.h"

core::ThreadedEventLoop2* EVENT_LOOP = nullptr;

static const char* TESTS[] = {
	"1.1.1", "1.1.2", "1.1.3", "1.1.4", "1.1.5", "1.1.6", "1.1.7", "1.1.8",
	"1.2.1", "1.2.2", "1.2.3", "1.2.4", "1.2.5", "1.2.6", "1.2.7", "1.2.8",

	"2.1", "2.2", "2.3", "2.4", "2.5", "2.6", "2.7", "2.8", "2.9", "2.10", "2.11",

	"3.1", "3.2", "3.3", "3.4", "3.5", "3.6", "3.7",

	"4.1.1", "4.1.2", "4.1.3", "4.1.4", "4.1.5", "4.2.1", "4.2.2", "4.2.3", "4.2.4", "4.2.5",

	"5.1", "5.2", "5.3", "5.4", "5.5", "5.6", "5.7", "5.8", "5.9", "5.10",
	"5.11", "5.12", "5.13", "5.14", "5.15", "5.16", "5.17", "5.18", "5.19", "5.20",

	"6.1.1", "6.1.2", "6.1.3", "6.2.1", "6.2.2", "6.2.3", "6.2.4", "6.3.1", "6.3.2",
	"6.4.1", "6.4.2", "6.4.3", "6.4.4", "6.5.1", "6.5.2", "6.5.3", "6.5.4", "6.5.5",
	"6.6.1", "6.6.2", "6.6.3", "6.6.4", "6.6.5", "6.6.6", "6.6.7", "6.6.8", "6.6.9",
	"6.6.10", "6.6.11", "6.7.1", "6.7.2", "6.7.3", "6.7.4", "6.8.1", "6.8.2", "6.9.1",
	"6.9.2", "6.9.3", "6.9.4", "6.10.1", "6.10.2", "6.10.3", "6.11.1", "6.11.2", "6.11.3",
	"6.11.4", "6.11.5", "6.12.1", "6.12.2", "6.12.3", "6.12.4", "6.12.5", "6.12.6", "6.12.7",
	"6.12.8", "6.13.1", "6.13.2", "6.13.3", "6.13.4", "6.13.5", "6.14.1", "6.14.2", "6.14.3",
	"6.14.4", "6.14.5", "6.14.6", "6.14.7", "6.14.8", "6.14.9", "6.14.10", "6.15.1", "6.16.1",
	"6.16.2", "6.16.3", "6.17.1", "6.17.2", "6.17.3", "6.17.4", "6.17.5", "6.18.1", "6.18.2",
	"6.18.3", "6.18.4", "6.18.5", "6.19.1", "6.19.2", "6.19.3", "6.19.4", "6.19.5", "6.20.1",
	"6.20.2", "6.20.3", "6.20.4", "6.20.5", "6.20.6", "6.20.7", "6.21.1", "6.21.2", "6.21.3",
	"6.21.4", "6.21.5", "6.21.6", "6.21.7", "6.21.8", "6.22.1", "6.22.2", "6.22.3", "6.22.4",
	"6.22.5", "6.22.6", "6.22.7", "6.22.8", "6.22.9", "6.22.10", "6.22.11", "6.22.12", "6.22.13",
	"6.22.14", "6.22.15", "6.22.16", "6.22.17", "6.22.18", "6.22.19", "6.22.20", "6.22.21",
	"6.22.22", "6.22.23", "6.22.24", "6.22.25", "6.22.26", "6.22.27", "6.22.28", "6.22.29",
	"6.22.30", "6.22.31", "6.22.32", "6.22.33", "6.22.34", "6.23.1", "6.23.2", "6.23.3",
	"6.23.4", "6.23.5", "6.23.6", "6.23.7",

	"7.1.1", "7.1.2", "7.1.3", "7.1.4", "7.1.5", "7.1.6", "7.3.1", "7.3.2", "7.3.3",
	"7.3.4", "7.3.5", "7.3.6", "7.5.1", "7.7.1", "7.7.2", "7.7.3", "7.7.4", "7.7.5",
	"7.7.6", "7.7.7", "7.7.8", "7.7.9", "7.7.10", "7.7.11", "7.7.12", "7.7.13", "7.9.1",
	"7.9.2", "7.9.3", "7.9.4", "7.9.5", "7.9.6", "7.9.7", "7.9.8", "7.9.9", "7.13.1", "7.13.2",

	"9.1.1", "9.1.2", "9.1.3", "9.1.4", "9.1.5", "9.1.6", "9.2.1", "9.2.2", "9.2.3", "9.2.4",
	"9.2.5", "9.2.6", "9.3.1", "9.3.2", "9.3.3", "9.3.4", "9.3.5", "9.3.6", "9.3.7", "9.3.8",
	"9.3.9", "9.4.1", "9.4.2", "9.4.3", "9.4.4", "9.4.5", "9.4.6", "9.4.7", "9.4.8", "9.4.9",
	"9.5.1", "9.5.2", "9.5.3", "9.5.4", "9.5.5", "9.5.6", "9.6.1", "9.6.2", "9.6.3", "9.6.4",
	"9.6.5", "9.6.6", "9.7.1", "9.7.2", "9.7.3", "9.7.4", "9.7.5", "9.7.6", "9.8.1", "9.8.2",
	"9.8.3", "9.8.4", "9.8.5", "9.8.6",

	"10.1.1"
};

void signalHandler(int signal)
{
	if (signal == SIGINT)
		EVENT_LOOP->stop();
}

class TestFinishedEvent: public core::Event2
{};

class ClientHandler: public core::EventThread2
{
	core::Allocator* m_allocator = nullptr;
	core::Shared<core::EventThread2> m_testDriver;
public:
	ClientHandler(core::EventLoop2* loop, core::Shared<core::EventThread2> testDriver, core::Allocator* allocator)
		: core::EventThread2(loop),
		  m_allocator(allocator),
		  m_testDriver(std::move(testDriver))
	{}

	core::HumanError handle(core::Event2* event) override
	{
		if (auto newConn = dynamic_cast<core::websocket::NewConnection3*>(event))
		{
			ZoneScopedN("NewConnection");
			return newConn->client()->startReadingMessages(sharedFromThis());
		}
		else if (auto messageEvent = dynamic_cast<core::websocket::MessageEvent*>(event))
		{
			ZoneScopedN("MessageEvent");
			if (messageEvent->message().type == core::websocket::Message::TYPE_TEXT)
			{
				return messageEvent->client()->writeText(core::StringView{messageEvent->message().payload});
			}
			else if (messageEvent->message().type == core::websocket::Message::TYPE_BINARY)
			{
				return messageEvent->client()->writeBinary(core::Span<const std::byte>{messageEvent->message().payload});
			}
			else
			{
				return messageEvent->handle();
			}
		}
		else if (auto errorEvent = dynamic_cast<core::websocket::ErrorEvent*>(event))
		{
			ZoneScopedN("ErrorEvent");
			return errorEvent->handle();
		}
		else if (auto disconnected = dynamic_cast<core::websocket::DisconnectedEvent*>(event))
		{
			ZoneScopedN("DisconnectedEvent");
			auto testFinished = unique_from<TestFinishedEvent>(m_allocator);
			(void)m_testDriver->send(std::move(testFinished));
			return stop();
		}
		return {};
	}
};

class ReportUpdatedEvent: public core::Event2
{};

class UpdateReportHandler: public core::EventThread2
{
	core::Allocator* m_allocator = nullptr;
	core::Shared<core::EventThread2> m_testDriver;
public:
	UpdateReportHandler(core::EventLoop2* loop, core::Shared<core::EventThread2> testDriver, core::Allocator* allocator)
		: core::EventThread2(loop),
		  m_allocator(allocator),
		  m_testDriver(std::move(testDriver))
	{}

	core::HumanError handle(core::Event2* event) override
	{
		if (auto newConn = dynamic_cast<core::websocket::NewConnection3*>(event))
		{
			ZoneScopedN("NewConnection");
			return newConn->client()->startReadingMessages(sharedFromThis());
		}
		else if (auto messageEvent = dynamic_cast<core::websocket::MessageEvent*>(event))
		{
			ZoneScopedN("MessageEvent");
			return messageEvent->handle();
		}
		else if (auto errorEvent = dynamic_cast<core::websocket::ErrorEvent*>(event))
		{
			ZoneScopedN("ErrorEvent");
			return errorEvent->handle();
		}
		else if (auto disconnected = dynamic_cast<core::websocket::DisconnectedEvent*>(event))
		{
			ZoneScopedN("DisconnectedEvent");
			auto reportUpdated = unique_from<ReportUpdatedEvent>(m_allocator);
			(void)m_testDriver->send(std::move(reportUpdated));
			return stop();
		}
		return {};
	}
};

class TestDriver: public core::EventThread2
{
	core::Allocator* m_allocator = nullptr;
	core::Log* m_log = nullptr;
	size_t m_currentTest = 0;
	size_t m_testsCount = sizeof(TESTS) / sizeof(*TESTS);
	core::StringView m_baseUrl;
	core::Shared<core::websocket::Client3> m_client;

	core::HumanError startTestClient()
	{
		auto testname = TESTS[m_currentTest];
		m_log->info("starting testcase {}"_sv, testname);
		auto urlWithPath = core::strf(m_allocator, "{}/runCase?casetuple={}&agent=taha2"_sv, m_baseUrl, testname);
		auto loop = eventLoop()->next();
		auto handler = loop->startThread<ClientHandler>(loop, sharedFromThis(), m_allocator);
		auto clientResult = core::websocket::Client3::connect(
			urlWithPath,
			2ULL * 1024ULL,
			64ULL * 1024ULL * 1024ULL,
			handler,
			m_log,
			m_allocator
		);
		if (clientResult.isError())
		{
			m_log->error("failed to connected client to '{}', {}"_sv, urlWithPath, clientResult.error());
			(void)handler->stop();
			return clientResult.releaseError();
		}
		m_client = clientResult.releaseValue();

		return {};
	}

	core::HumanError startUpdateReportClient()
	{
		auto urlWithPath = core::strf(m_allocator, "{}/updateReports?agent=taha2"_sv, m_baseUrl);
		auto loop = eventLoop()->next();
		auto handler = loop->startThread<UpdateReportHandler>(loop, sharedFromThis(), m_allocator);
		auto clientResult = core::websocket::Client3::connect(
			urlWithPath,
			2ULL * 1024ULL,
			64ULL * 1024ULL * 1024ULL,
			handler,
			m_log,
			m_allocator
		);
		if (clientResult.isError())
		{
			m_log->error("failed to connected client to '{}', {}"_sv, urlWithPath, clientResult.error());
			(void)handler->stop();
			return clientResult.releaseError();
		}
		m_client = clientResult.releaseValue();

		return {};
	}

public:
	TestDriver(core::EventLoop2* loop, core::StringView baseUrl, core::Log* log, core::Allocator* allocator)
		: core::EventThread2(loop),
		  m_allocator(allocator),
		  m_log(log),
		  m_baseUrl(baseUrl)
	{}

	~TestDriver() override
	{
		eventLoop()->stopAllLoops();
	}

	core::HumanError handle(core::Event2* event) override
	{
		if (auto startEvent = dynamic_cast<core::StartEvent2*>(event))
		{
			ZoneScopedN("TestDriver::StartEvent");
			return startTestClient();
		}
		else if (auto testFinished = dynamic_cast<TestFinishedEvent*>(event))
		{
			ZoneScopedN("TestDriver::TestFinishedEvent");
			return startUpdateReportClient();
		}
		else if (auto reportUpdated = dynamic_cast<ReportUpdatedEvent*>(event))
		{
			ZoneScopedN("TestDriver::ReportUpdatedEvent");
			++m_currentTest;
			if (m_currentTest < m_testsCount)
			{
				return startTestClient();
			}
			else
			{
				m_client = nullptr;
				return stop();
			}
		}
		return {};
	}
};

int main(int argc, char** argv)
{
	auto url = "ws://localhost:9011"_sv;

	if (argc > 1)
		url = core::StringView{argv[1]};

	signal(SIGINT, signalHandler);

	core::FastLeak allocator;
	core::Log log{&allocator};

	auto threadedEventLoopResult = core::ThreadedEventLoop2::create(&log, &allocator);
	if (threadedEventLoopResult.isError())
	{
		log.critical("failed to create threaded event loop, {}"_sv, threadedEventLoopResult.releaseError());
		return EXIT_FAILURE;
	}
	auto threadedEventLoop = threadedEventLoopResult.releaseValue();
	EVENT_LOOP = threadedEventLoop.get();

	auto loop = threadedEventLoop->next();
	loop->startThread<TestDriver>(loop, url, &log, &allocator);

	auto err = threadedEventLoop->run();
	if (err)
	{
		log.critical("event loop error, {}"_sv, err);
		return EXIT_FAILURE;
	}

	log.debug("success"_sv);
	return 0;
}
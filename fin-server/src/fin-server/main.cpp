#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/EventLoop.h>

#include <fin/Server.h>

auto HELP = R"""(fin-server finapp local server
fin-server path/to/ledger/file [OPTIONS]
OPTIONS:
  --help, -h: prints this message
  --create, -c: creates the ledger file if it doesn't exist
)"""_sv;

struct Option
{
	core::StringView name;
	core::StringView& value;
	bool required = true;
};

class Args
{
	core::Allocator* m_allocator = nullptr;
	core::StringView m_file;
	bool m_help = false;
	bool m_create = false;

	explicit Args(core::Allocator* allocator)
		: m_allocator(allocator)
	{}

public:
	static core::Result<Args> parse(int argc, char** argv, core::Allocator* allocator)
	{
		if (argc < 2)
			return core::errf(allocator, "no command found"_sv);

		// special case the help command
		Args res{allocator};
		res.m_file = core::StringView{argv[1]};
		if (res.m_file == "--help"_sv || res.m_file == "-h"_sv)
		{
			res.m_file = {};
			res.m_help = true;
			return res;
		}

		for (int i = 2; i < argc; ++i)
		{
			auto option = core::StringView{argv[i]};
			if (option.startsWith("-"_sv))
			{
				option = option.trimLeft("-"_sv).trim();

				if (option == "help"_sv || option == "h"_sv)
				{
					res.m_help = true;
				}
				else if (option == "create"_sv || option == "c"_sv)
				{
					res.m_create = true;
				}
				else
				{
					return core::errf(allocator, "unknown option, {}"_sv, option);
				}
			}
			else
			{
				return core::errf(allocator, "unknown option, {}"_sv, option);
			}
		}
		return res;
	}

	core::StringView file() const { return m_file; }
	bool help() const { return m_help; }
	bool create() const { return m_create; }
};

int main(int argc, char** argv)
{
	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto argsResult = Args::parse(argc, argv, &allocator);
	if (argsResult.isError())
	{
		log.critical("failed to parse cli arguments, {}"_sv, argsResult.releaseError());
		return EXIT_FAILURE;
	}
	auto args = argsResult.releaseValue();

	if (args.help())
	{
		log.info("{}"_sv, HELP);
		return EXIT_SUCCESS;
	}
	else
	{
		auto threadedEventLoopResult = core::ThreadedEventLoop::create(&log, &allocator);
		if (threadedEventLoopResult.isError())
		{
			log.critical("failed to create event loop, {}"_sv, threadedEventLoopResult.releaseError());
			return EXIT_FAILURE;
		}
		auto threadedEventLoop = threadedEventLoopResult.releaseValue();

		auto lockInfoResult = fin::LockInfo::create(args.file(), &allocator);
		if (lockInfoResult.isError())
		{
			log.critical("{}"_sv, lockInfoResult.releaseError());
			return EXIT_FAILURE;
		}
		auto lockInfo = lockInfoResult.releaseValue();
		log.info("absPath: {}, lockName: {}, filePath: {}"_sv, lockInfo.absPath(), lockInfo.lockName(), lockInfo.tmpFilePath());

		auto serverResult = fin::Server::create(
			args.file(),
			""_sv,
			fin::Server::FLAG::CREATE_LEDGER,
			threadedEventLoop->next(),
			&log,
			&allocator
		);
		if (serverResult.isError())
		{
			log.critical("failed to create server instance, {}"_sv, serverResult.releaseError());
			return EXIT_FAILURE;
		}
		auto server = serverResult.releaseValue();

		log.info("listening on: {}"_sv, server->listeningPort());

		auto err = threadedEventLoop->run();
		if (err)
		{
			log.critical("event loop error, {}"_sv, err);
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}
}
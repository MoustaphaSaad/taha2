#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/StringView.h>

#include <budget-byte/Ledger.h>

auto HELP = R"""(budget-byte-cli the cli interface for budget byte financial tracker
budget-byte-cli path/to/ledger/file command [options]
COMMANDS:
  help: prints this message
    - budget-byte-cli help
  init: creates a new ledger file
    - budget-byte-cli path/to/ledger/file init
)"""_sv;

class Args
{
	core::StringView m_file;
	core::StringView m_command;
public:
	static core::Result<Args> parse(int argc, char** argv, core::Allocator* allocator)
	{
		if (argc < 2)
			return core::errf(allocator, "no command found"_sv);

		// special case the help command
		Args res{};
		res.m_file = core::StringView{argv[1]};
		if (res.m_file == "help"_sv)
		{
			res.m_command = res.m_file;
			res.m_file = {};
			return res;
		}

		if (argc < 3)
			return core::errf(allocator, "no command found"_sv);

		res.m_command = core::StringView{argv[2]};
		return res;
	}

	core::StringView file() const { return m_file; }
	core::StringView command() const { return m_command; }
};

int main(int argc, char** argv)
{
	core::Mallocator allocator{};
	core::Log log{&allocator};

	auto argsResult = Args::parse(argc, argv, &allocator);
	if (argsResult.isError())
	{
		log.critical("failed to parse cli arguments, {}"_sv, argsResult.releaseError());
		return EXIT_FAILURE;
	}
	auto args = argsResult.releaseValue();

	if (args.command() == "help"_sv)
	{
		log.info("{}"_sv, HELP);
		return EXIT_SUCCESS;
	}
	else if (args.command() == "init"_sv)
	{
		auto ledgerResult = budget::Ledger::open(args.file(), &allocator);
		if (ledgerResult.isError())
		{
			log.critical("failed to create ledger file, {}"_sv, ledgerResult.releaseError());
			return EXIT_FAILURE;
		}
		log.info("ledger file {} created"_sv, args.file());
		return EXIT_SUCCESS;
	}
	return EXIT_SUCCESS;
}
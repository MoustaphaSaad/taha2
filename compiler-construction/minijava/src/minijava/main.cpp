#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Result.h>
#include <core/Array.h>
#include <core/StringView.h>
#include <core/File.h>

#include "minijava/Unit.h"

constexpr auto HELP = R"""(MiniJava compiler usage:
minijava COMMAND [files...]
COMMANDS:
	help: displays this help message
	scan: scans the given files and prints the found tokens
)""";

class Args
{
	core::StringView m_command;
	core::Array<core::StringView> m_files;

	Args(core::StringView command, core::Array<core::StringView> files)
		: m_command(command),
		  m_files(std::move(files))
	{}
public:
	static core::Result<Args> parse(int argc, char* argv[], core::Allocator* allocator)
	{
		if (argc <= 1)
			return core::errf(allocator, "no command specified"_sv);

		auto command = core::StringView{argv[1]};
		core::Array<core::StringView> files{allocator};

		for (size_t i = 2; i < argc; ++i)
			files.push(core::StringView{argv[i]});

		return Args{command, std::move(files)};
	}

	core::StringView command() const { return m_command; }
	const core::Array<core::StringView>& files() const { return m_files; }
};

int main(int argc, char* argv[])
{
	core::FastLeak allocator{};
	core::Log log{&allocator};

	auto argsResult = Args::parse(argc, argv, &allocator);
	if (argsResult.isError())
	{
		log.critical("{}"_sv, argsResult.releaseError());
		return EXIT_FAILURE;
	}
	auto args = argsResult.releaseValue();

	if (args.command().equalsIgnoreCase("help"_sv))
	{
		core::strf(core::File::STDOUT, "{}"_sv, HELP);
		return EXIT_SUCCESS;
	}
	else if (args.command().equalsIgnoreCase("scan"_sv))
	{
		for (const auto& file: args.files())
		{
			auto unitResult = minijava::Unit::create(file, &allocator);
			if (unitResult.isError())
			{
				core::strf(core::File::STDOUT, "{}\n"_sv, unitResult.releaseError());
				continue;
			}
			auto unit = unitResult.releaseValue();

			if (unit->scan() == false)
				unit->dumpErrors(core::File::STDOUT);
			else
				unit->dumpTokens(core::File::STDOUT);
		}

		return EXIT_SUCCESS;
	}
	else
	{
		log.critical("unknown command '{}'"_sv, args.command());
		return EXIT_FAILURE;
	}
}
#include <core/FastLeak.h>
#include <core/Log.h>
#include <core/Result.h>
#include <core/Array.h>
#include <core/StringView.h>
#include <core/File.h>
#include <core/Hash.h>
#include <core/MemoryStream.h>

#include <fmt/color.h>

#include "minijava/Unit.h"
#include "minijava/Scanner.h"
#include "minijava/Parser.h"

constexpr auto HELP = R"""(MiniJava compiler usage:
minijava COMMAND [OPTIONS] [files...]
COMMANDS:
	help: displays this help message
	scan: scans the given files and prints the found tokens
		--test, -t: runs the compiler in test mode where it loads the .out
					of the given file and compares the output to it.
	parse-expr: parses an expression from the given file
		--test, -t: runs the compiler in test mode where it loads the .out
					of the given file and compares the output to it.
)""";

class Args
{
	core::StringView m_command;
	core::Array<core::StringView> m_files;
	core::Map<core::StringView, core::StringView> m_options;

	Args(core::StringView command, core::Array<core::StringView> files, core::Map<core::StringView, core::StringView> options)
		: m_command(command),
		  m_files(std::move(files)),
		  m_options(std::move(options))
	{}
public:
	static core::Result<Args> parse(int argc, char* argv[], core::Allocator* allocator)
	{
		if (argc <= 1)
			return core::errf(allocator, "no command specified"_sv);

		auto command = core::StringView{argv[1]};
		core::Array<core::StringView> files{allocator};
		core::Map<core::StringView, core::StringView> options{allocator};

		if (command == "scan"_sv)
		{
			for (size_t i = 2; i < argc; ++i)
			{
				core::StringView arg{argv[i]};
				if (arg == "--test"_sv || arg == "-t"_sv)
					options.insert(arg, ""_sv);
				else
					files.push(arg);
			}
		}
		else if (command == "parse-expr"_sv)
		{
			for (size_t i = 2; i < argc; ++i)
			{
				core::StringView arg{argv[i]};
				if (arg == "--test"_sv || arg == "-t"_sv)
					options.insert(arg, ""_sv);
				else
					files.push(arg);
			}
		}

		return Args{command, std::move(files), std::move(options)};
	}

	core::StringView command() const { return m_command; }
	const core::Array<core::StringView>& files() const { return m_files; }
	bool hasOption(core::StringView opt) const { return m_options.lookup(opt) != m_options.end(); }
	core::StringView getOption(core::StringView opt) const { return m_options.lookup(opt)->value; }
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

			if (args.hasOption("--test"_sv) || args.hasOption("-t"_sv))
			{
				auto expectedOutputFile = core::strf(&allocator, "{}.out"_sv, file);
				auto expectedOutputResult = core::File::content(&allocator, expectedOutputFile);
				if (expectedOutputResult.isError())
				{
					log.critical("failed to load expected output file, {}"_sv, expectedOutputResult.releaseError());
					return EXIT_FAILURE;
				}
				auto expectedOutput = expectedOutputResult.releaseValue();

				core::MemoryStream outputStream{&allocator};

				if (unit->scan() == false)
					unit->dumpErrors(&outputStream);
				else
					unit->dumpTokens(&outputStream);
				auto output = outputStream.releaseString();
				if (output != expectedOutput)
				{
					core::strf(core::File::STDOUT, "[{}]: {}"_sv, fmt::styled("FAIL", fmt::fg(fmt::color::red)), file);
					return EXIT_FAILURE;
				}
				else
				{
					core::strf(core::File::STDOUT, "[{}]: {}"_sv, fmt::styled("PASS", fmt::fg(fmt::color::green)), file);
					return EXIT_SUCCESS;
				}
			}
			else
			{
				if (unit->scan() == false)
					unit->dumpErrors(core::File::STDOUT);
				else
					unit->dumpTokens(core::File::STDOUT);
			}
		}

		return EXIT_SUCCESS;
	}
	else if (args.command().equalsIgnoreCase("parse-expr"_sv))
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

			if (args.hasOption("--test"_sv) || args.hasOption("-t"_sv))
			{
				auto expectedOutputFile = core::strf(&allocator, "{}.out"_sv, file);
				auto expectedOutputResult = core::File::content(&allocator, expectedOutputFile);
				if (expectedOutputResult.isError())
				{
					log.critical("failed to load expected output file, {}"_sv, expectedOutputResult.releaseError());
					return EXIT_FAILURE;
				}
				auto expectedOutput = expectedOutputResult.releaseValue();

				core::MemoryStream outputStream{&allocator};

				if (unit->scan() == false)
				{
					unit->dumpErrors(&outputStream);
					return EXIT_FAILURE;
				}

				minijava::Parser parser{unit.get(), &allocator};
				auto expr = parser.parseExpr();

				// TODO: dump the expression AST

				auto output = outputStream.releaseString();
				if (output != expectedOutput)
				{
					core::strf(core::File::STDOUT, "[{}]: {}"_sv, fmt::styled("FAIL", fmt::fg(fmt::color::red)), file);
					return EXIT_FAILURE;
				}
				else
				{
					core::strf(core::File::STDOUT, "[{}]: {}"_sv, fmt::styled("PASS", fmt::fg(fmt::color::green)), file);
					return EXIT_SUCCESS;
				}
			}
			else
			{
				if (unit->scan() == false)
					unit->dumpErrors(core::File::STDOUT);
				else
					unit->dumpTokens(core::File::STDOUT);
			}
		}

		return EXIT_SUCCESS;
	}
	else
	{
		log.critical("unknown command '{}'"_sv, args.command());
		return EXIT_FAILURE;
	}
}
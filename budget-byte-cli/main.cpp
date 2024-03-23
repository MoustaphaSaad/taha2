#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/StringView.h>
#include <core/Hash.h>

#include <budget-byte/Ledger.h>

#include <fmt/chrono.h>

auto HELP = R"""(budget-byte-cli the cli interface for budget byte financial tracker
budget-byte-cli path/to/ledger/file command [options]
COMMANDS:
  help: prints this message
    - budget-byte-cli help
  init: creates a new ledger file
    - budget-byte-cli path/to/ledger/file init
  add-account: adds a new account to the ledger
    - budget-byte-cli path/to/ledger/file add-account -name account_name
  add: adds a new transaction to the ledger
    - budget-byte-cli path/to/ledger/file add -amount 123 -src account -dst account -date YYYY-MM-DD -notes "your notes goes here"
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
	core::StringView m_command;
	core::Map<core::StringView, core::StringView> m_options;

	Args(core::Allocator* allocator)
		: m_allocator(allocator),
		  m_options(allocator)
	{}

public:
	static core::Result<Args> parse(int argc, char** argv, core::Allocator* allocator)
	{
		if (argc < 2)
			return core::errf(allocator, "no command found"_sv);

		// special case the help command
		Args res{allocator};
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
		for (int i = 3; i < argc; ++i)
		{
			auto option = core::StringView{argv[i]};
			if (option.startsWith("-"_sv))
			{
				option = option.slice(1, option.count()).trim();
				if (i + 1 < argc)
				{
					if (res.hasOption(option))
						return core::errf(allocator, "option {} is already defined"_sv, option);

					auto value = core::StringView{argv[i + 1]}.trim();
					++i;
					res.m_options.insert(option, value);
				}
				else
				{
					return core::errf(allocator, "option {} has no value"_sv, option);
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
	core::StringView command() const { return m_command; }
	bool hasOption(core::StringView key) const { return m_options.lookup(key) != m_options.end(); }
	core::StringView getOption(core::StringView key) const { return m_options.lookup(key)->value; }
	core::HumanError loadOptions(std::initializer_list<Option> options) const
	{
		for (auto& option: options)
		{
			if (hasOption(option.name))
			{
				option.value = getOption(option.name);
			}
			else if (option.required)
			{
				return core::errf(m_allocator, "required option '{}' doesn't exist"_sv, option.name);
			}
		}
		return {};
	}
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
	else if (args.command() == "add-account"_sv)
	{
		auto ledgerResult = budget::Ledger::open(args.file(), &allocator);
		if (ledgerResult.isError())
		{
			log.critical("failed to open ledger file, {}"_sv, ledgerResult.releaseError());
			return EXIT_FAILURE;
		}
		auto ledger = ledgerResult.releaseValue();

		core::StringView name;
		auto err = args.loadOptions({
			{"name"_sv, name},
		});
		if (err)
		{
			log.critical("failed to parse add-account command arguments, {}"_sv, err);
			return EXIT_FAILURE;
		}

		err = ledger.addAccount(name);
		if (err)
		{
			log.critical("failed to add account, {}"_sv, err);
			return EXIT_FAILURE;
		}

		log.info("Success, name = {}, balance = 0"_sv, name);
		return EXIT_SUCCESS;
	}
	else if (args.command() == "add"_sv)
	{
		auto ledgerResult = budget::Ledger::open(args.file(), &allocator);
		if (ledgerResult.isError())
		{
			log.critical("failed to open ledger file, {}"_sv, ledgerResult.releaseError());
			return EXIT_FAILURE;
		}
		auto ledger = ledgerResult.releaseValue();

		core::StringView amount, src, dst, date, notes;
		auto err = args.loadOptions({
			{"amount"_sv, amount},
			{"src"_sv, src},
			{"dst"_sv, dst},
			{"date"_sv, date, false},
			{"notes"_sv, notes, false},
		});
		if (err)
		{
			log.critical("failed to parse add command arguments, {}"_sv, err);
			return EXIT_FAILURE;
		}

		uint64_t parsedAmount = 0;
		auto res = std::from_chars(amount.begin(), amount.end(), parsedAmount);
		if (res.ec != std::errc())
		{
			log.critical("failed to parse amount value as positive integer"_sv);
			return EXIT_FAILURE;
		}

		std::chrono::year_month_day parsedDate = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
		if (date.count() > 0)
		{
			std::istringstream str{date.data()};
			str >> std::chrono::parse("%F", parsedDate);
			if (str.fail())
			{
				log.critical("failed to parse date {}, it should be in YYYY-MM-DD format"_sv, date);
				return EXIT_FAILURE;
			}
		}

		err = ledger.addTransaction(parsedAmount, src, dst, parsedDate, notes);
		if (err)
		{
			log.critical("{}"_sv, err);
			return EXIT_FAILURE;
		}

		auto tm = std::chrono::local_days{parsedDate};
		log.info("Success, amount = {}, src = {}, dst = {}, date = {:%Y-%m-%d}, notes = {}"_sv, parsedAmount, src, dst, tm, notes);
		return EXIT_SUCCESS;
	}
	return EXIT_SUCCESS;
}
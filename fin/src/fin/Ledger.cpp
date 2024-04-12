#include "fin/Ledger.h"

#include <fmt/chrono.h>

namespace fin
{
	Ledger::Ledger(sqlite3* db, core::String path, core::Allocator* allocator)
		: m_allocator(allocator),
		  m_path(std::move(path)),
		  m_db(db)
	{}

	core::Result<Ledger> Ledger::open(core::StringView path, core::Allocator* allocator)
	{
		sqlite3* db = nullptr;

		auto cPath = core::String{path, allocator};
		auto rc = sqlite3_open_v2(cPath.data(), &db, DB_FLAG_READWRITE | DB_FLAG_CREATE | DB_FLAG_NOMUTEX | DB_FLAG_EXCLUSIVE, nullptr);
		if (rc != SQLITE_OK)
			return core::errf(allocator, "failed to open sqlite3 database, {}"_sv, sqlite3_errstr(rc));
		auto res = Ledger{db, std::move(cPath), allocator};

		// setup the db file
		const char* INIT_QUERY = R"QUERY(
			PRAGMA journal_mode = WAL;
			PRAGMA synchronous = NORMAL;
			PRAGMA user_version = 1;

			CREATE TABLE IF NOT EXISTS "accounts" (
				name TEXT PRIMARY KEY,
				balance INTEGER NOT NULL
			);

			CREATE TABLE IF NOT EXISTS "ledger" (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				amount INTEGER NOT NULL,
				src TEXT NOT NULL,
				dst TEXT NOT NULL,
				date TEXT NOT NULL,
				notes TEXT,
				FOREIGN KEY(src) REFERENCES accounts(name),
				FOREIGN KEY(dst) REFERENCES accounts(name)
				CHECK (src <> dst)
			);

			INSERT OR IGNORE INTO accounts(name, balance) VALUES('init', 0);
		)QUERY";

		char* sqlerr = nullptr;
		rc = sqlite3_exec(res.m_db, INIT_QUERY, nullptr, nullptr, &sqlerr);
		if (rc != SQLITE_OK)
		{
			auto res = core::errf(allocator, "failed to init sqlite3 database, {}"_sv, sqlerr);
			::free(sqlerr);
			return res;
		}

		const char* ADD_TRANSACTION_QUERY = R"QUERY(
			INSERT INTO ledger(amount, src, dst, date, notes) VALUES(?1, ?2, ?3, ?4, ?5)
		)QUERY";
		rc = sqlite3_prepare_v3(res.m_db, ADD_TRANSACTION_QUERY, (int)::strlen(ADD_TRANSACTION_QUERY), 0, &res.m_addTransactionStmt, nullptr);
		if (rc != SQLITE_OK)
			return core::errf(allocator, "failed to prepare add sql stmt, {}"_sv, sqlite3_errstr(rc));

		const char* ADD_ACCOUNT_QUERY = R"QUERY(
			INSERT INTO accounts(name, balance) VALUES(?1, ?2)
		)QUERY";
		rc = sqlite3_prepare_v3(res.m_db, ADD_ACCOUNT_QUERY, (int)::strlen(ADD_ACCOUNT_QUERY), 0, &res.m_addAccountStmt, nullptr);
		if (rc != SQLITE_OK)
			return core::errf(allocator, "failed to prepare add sql stmt, {}"_sv, sqlite3_errstr(rc));

		return res;
	}

	core::HumanError Ledger::addAccount(core::StringView name)
	{
		auto rc = sqlite3_reset(m_addAccountStmt);
		if (rc != SQLITE_OK)
			return core::errf(m_allocator, "failed to add account, {}"_sv, sqlite3_errstr(rc));

		auto cName = core::String{name, m_allocator};
		rc = sqlite3_bind_text(m_addAccountStmt, 1, cName.data(), (int)cName.count(), SQLITE_STATIC);
		if (rc != SQLITE_OK)
			return core::errf(m_allocator, "failed to add account, {}"_sv, sqlite3_errstr(rc));

		rc = sqlite3_bind_int64(m_addAccountStmt, 2, 0);
		if (rc != SQLITE_OK)
			return core::errf(m_allocator, "failed to add account, {}"_sv, sqlite3_errstr(rc));

		rc = sqlite3_step(m_addAccountStmt);
		if (rc != SQLITE_DONE)
			return core::errf(m_allocator, "failed to add account, {}"_sv, sqlite3_errstr(rc));

		return {};
	}

	core::HumanError Ledger::addTransaction(
		uint64_t amount,
		core::StringView src_account,
		core::StringView dst_account,
		std::chrono::year_month_day date,
		core::StringView notes
	)
	{
		auto rc = sqlite3_reset(m_addTransactionStmt);
		if (rc != SQLITE_OK)
			return core::errf(m_allocator, "failed to add transaction, {}"_sv, sqlite3_errstr(rc));

		rc = sqlite3_bind_int64(m_addTransactionStmt, 1, amount);
		if (rc != SQLITE_OK)
			return core::errf(m_allocator, "failed to add transaction, {}"_sv, sqlite3_errstr(rc));

		auto cSrc = core::String{src_account, m_allocator};
		rc = sqlite3_bind_text(m_addTransactionStmt, 2, cSrc.data(), (int)cSrc.count(), SQLITE_STATIC);
		if (rc != SQLITE_OK)
			return core::errf(m_allocator, "failed to add transaction, {}"_sv, sqlite3_errstr(rc));

		auto cDst = core::String{dst_account, m_allocator};
		rc = sqlite3_bind_text(m_addTransactionStmt, 3, cDst.data(), (int)cDst.count(), SQLITE_STATIC);
		if (rc != SQLITE_OK)
			return core::errf(m_allocator, "failed to add transaction, {}"_sv, sqlite3_errstr(rc));

		auto dateStr = core::strf(m_allocator, "{:%Y-%m-%d}"_sv, std::chrono::sys_days{date});
		rc = sqlite3_bind_text(m_addTransactionStmt, 4, dateStr.data(), (int)dateStr.count(), SQLITE_STATIC);
		if (rc != SQLITE_OK)
			return core::errf(m_allocator, "failed to add transaction, {}"_sv, sqlite3_errstr(rc));

		auto cNotes = core::String{notes, m_allocator};
		rc = sqlite3_bind_text(m_addTransactionStmt, 5, cNotes.data(), (int)cNotes.count(), SQLITE_STATIC);
		if (rc != SQLITE_OK)
			return core::errf(m_allocator, "failed to add transaction, {}"_sv, sqlite3_errstr(rc));

		rc = sqlite3_step(m_addTransactionStmt);
		if (rc != SQLITE_DONE)
			return core::errf(m_allocator, "failed to add transaction, {}"_sv, sqlite3_errstr(rc));

		return {};
	}
}
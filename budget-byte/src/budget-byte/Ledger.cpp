#include "budget-byte/Ledger.h"

namespace budget
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

		// setup the db file
		const char* INIT_QUERY = R"QUERY(
			PRAGMA journal_mode = WAL;
			PRAGMA synchronous = NORMAL;
			PRAGMA user_version = 1;

			CREATE TABLE IF NOT EXISTS "accounts" (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				name TEXT UNIQUE NOT NULL,
				balance INTEGER NOT NULL
			);

			CREATE TABLE IF NOT EXISTS "ledger" (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				amount INTEGER NOT NULL,
				src_id INTEGER NOT NULL,
				dst_id INTEGER NOT NULL,
				date TEXT NOT NULL,
				notes TEXT,
				FOREIGN KEY(src_id) REFERENCES accounts(id),
				FOREIGN KEY(dst_id) REFERENCES accounts(id)
				CHECK (src_id <> dst_id)
			);

			INSERT OR IGNORE INTO accounts(name, balance) VALUES('init', 0);
		)QUERY";

		char* sqlerr = nullptr;
		rc = sqlite3_exec(db, INIT_QUERY, nullptr, nullptr, &sqlerr);
		if (rc != SQLITE_OK)
		{
			auto res = core::errf(allocator, "failed to init sqlite3 database, {}"_sv, sqlerr);
			::free(sqlerr);
			sqlite3_close_v2(db);
			return res;
		}

		return Ledger{db, std::move(cPath), allocator};
	}
}
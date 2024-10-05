#include "ledger/Ledger.h"

namespace ledger
{
	Ledger::Ledger(sqlite3* db, core::String path, core::Allocator* allocator)
		: m_allocator(allocator),
		  m_path(std::move(path)),
		  m_db(db)
	{}

	core::Result<sqlite3_stmt*> Ledger::createStmt(core::StringView query)
	{
		sqlite3_stmt* res = nullptr;
		auto rc = sqlite3_prepare_v3(m_db, query.data(), query.count(), 0, &res, nullptr);
		if (rc != SQLITE_OK)
		{
			return core::errf(m_allocator, "sqlite3_prepare_v3 failed, {}"_sv, sqlite3_errstr(rc));
		}
		return res;
	}

	core::Result<Ledger> Ledger::open(core::StringView path, core::Allocator* allocator)
	{
		sqlite3* db = nullptr;

		auto cPath = core::String{path, allocator};
		auto rc = sqlite3_open_v2(
			cPath.data(),
			&db,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_EXCLUSIVE,
			nullptr);
		if (rc != SQLITE_OK)
		{
			return core::errf(allocator, "sqlite3_open_v2 failed, {}"_sv, sqlite3_errstr(rc));
		}
		auto ledger = Ledger{db, core::String{path, allocator}, allocator};

		struct SQLStmt
		{
			sqlite3_stmt*& stmt;
			core::StringView query;
		};

		SQLStmt stmts[] = {
			SQLStmt{ledger.m_getAppID, "PRAGMA application_id"_sv},
			SQLStmt{ledger.m_getUserVersion, "PRAGMA user_version"_sv},
		};

		for (auto s: stmts)
		{
			auto res = ledger.createStmt(s.query);
			if (res.isError())
			{
				return res.releaseError();
			}
			s.stmt = res.releaseValue();
		}

		auto versionResult = ledger.queryVersion();
		if (versionResult.isError())
		{
			return versionResult.releaseError();
		}
		auto version = versionResult.releaseValue();

		if (version.appID == 0 && version.userVersion == 0)
		{
			// db file is newly created, initialize it
		}
		else if (version.appID == VERSION.appID)
		{
			// db file is initialized, check the version here and open it
		}

		return ledger;
	}

	core::Result<Ledger::Version> Ledger::queryVersion()
	{
		// get app id
		auto rc = sqlite3_reset(m_getAppID);
		if (rc != SQLITE_OK)
		{
			return core::errf(m_allocator, "sqlite3_reset failed, {}"_sv, sqlite3_errstr(rc));
		}

		rc = sqlite3_step(m_getAppID);
		if (rc != SQLITE_DONE)
		{
			return core::errf(m_allocator, "sqlite3_step failed, {}"_sv, sqlite3_errstr(rc));
		}

		auto appID = sqlite3_column_int(m_getAppID, 0);

		// get user version
		rc = sqlite3_reset(m_getUserVersion);
		if (rc != SQLITE_OK)
		{
			return core::errf(m_allocator, "sqlite3_reset failed, {}"_sv, sqlite3_errstr(rc));
		}

		rc = sqlite3_step(m_getUserVersion);
		if (rc != SQLITE_DONE)
		{
			return core::errf(m_allocator, "sqlite3_step failed, {}"_sv, sqlite3_errstr(rc));
		}

		auto userVersion = sqlite3_column_int(m_getUserVersion, 0);

		return Ledger::Version{
			.appID = appID,
			.userVersion = userVersion,
		};
	}
}
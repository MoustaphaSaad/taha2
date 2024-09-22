#include "ledger/Ledger.h"

namespace ledger
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
		auto rc = sqlite3_open_v2(cPath.data(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_EXCLUSIVE, nullptr);
		if (rc != SQLITE_OK)
			return core::errf(allocator, "sqlite3_open_v2 failed, {}"_sv, sqlite3_errstr(rc));
		coreDefer {sqlite3_close(db);};

		auto res = Ledger{db, core::String{path, allocator}, allocator};
		db = nullptr;
		return res;
	}
}
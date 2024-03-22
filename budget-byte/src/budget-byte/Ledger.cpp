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

		auto c_path = core::String{path, allocator};
		auto rc = sqlite3_open_v2(c_path.data(), &db, DB_FLAG_READWRITE | DB_FLAG_CREATE | DB_FLAG_NOMUTEX | DB_FLAG_EXCLUSIVE, nullptr);
		if (rc != SQLITE_OK)
			return core::errf(allocator, "failed to open sqlite3 database, {}"_sv, sqlite3_errstr(rc));

		return Ledger{db, std::move(c_path), allocator};
	}
}
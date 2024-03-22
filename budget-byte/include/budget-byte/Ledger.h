#pragma once

#include "budget-byte/Exports.h"

#include <core/Result.h>
#include <core/StringView.h>

#include <sqlite3.h>

namespace budget
{
	class Ledger
	{
		core::Allocator* m_allocator = nullptr;
		core::String m_path;
		sqlite3* m_db = nullptr;

		void destroy()
		{
			if (m_db)
			{
				sqlite3_close_v2(m_db);
				m_db = nullptr;
			}
		}

		void moveFrom(Ledger&& other)
		{
			m_db = other.m_db;
			other.m_db = nullptr;
		}

		Ledger(sqlite3* db, core::String path, core::Allocator* allocator);
	public:
		enum DB_FLAG
		{
			DB_FLAG_READONLY = SQLITE_OPEN_READONLY,
			DB_FLAG_READWRITE = SQLITE_OPEN_READWRITE,
			DB_FLAG_CREATE = SQLITE_OPEN_CREATE,
			DB_FLAG_EXCLUSIVE = SQLITE_OPEN_EXCLUSIVE,
			DB_FLAG_FULLMUTEX = SQLITE_OPEN_FULLMUTEX,
			DB_FLAG_NOMUTEX = SQLITE_OPEN_NOMUTEX,
		};

		BUDGET_BYTE_EXPORT static core::Result<Ledger> open(core::StringView path, core::Allocator* allocator);

		Ledger(const Ledger&) = delete;
		Ledger& operator=(const Ledger&) = delete;

		Ledger(Ledger&& other)
			: m_path(other.m_allocator)
		{
			moveFrom(std::move(other));
		}

		Ledger& operator=(Ledger&& other)
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~Ledger()
		{
			destroy();
		}
	};
}
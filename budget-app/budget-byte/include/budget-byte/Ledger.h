#pragma once

#include "budget-byte/Exports.h"

#include <core/Result.h>
#include <core/StringView.h>

#include <sqlite3.h>

#include <chrono>

namespace budget
{
	class Ledger
	{
		core::Allocator* m_allocator = nullptr;
		core::String m_path;
		sqlite3* m_db = nullptr;
		sqlite3_stmt* m_addTransactionStmt = nullptr;
		sqlite3_stmt* m_addAccountStmt = nullptr;

		void destroy()
		{
			if (m_addAccountStmt)
			{
				sqlite3_finalize(m_addAccountStmt);
				m_addAccountStmt = nullptr;
			}

			if (m_addTransactionStmt)
			{
				sqlite3_finalize(m_addTransactionStmt);
				m_addTransactionStmt = nullptr;
			}

			if (m_db)
			{
				sqlite3_close_v2(m_db);
				m_db = nullptr;
			}
		}

		void moveFrom(Ledger&& other)
		{
			m_allocator = other.m_allocator;
			m_path = std::move(other.m_path);
			m_db = other.m_db;
			m_addTransactionStmt = other.m_addTransactionStmt;
			m_addAccountStmt = other.m_addAccountStmt;
			other.m_allocator = nullptr;
			other.m_db = nullptr;
			other.m_addTransactionStmt = nullptr;
			other.m_addAccountStmt = nullptr;
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

		BUDGET_BYTE_EXPORT core::HumanError addAccount(core::StringView name);

		BUDGET_BYTE_EXPORT core::HumanError addTransaction(
			uint64_t amount,
			core::StringView src_account,
			core::StringView dst_account,
			std::chrono::year_month_day date,
			core::StringView notes = ""_sv);
	};
}
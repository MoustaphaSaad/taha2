#pragma once

#include "ledger/Exports.h"

#include <core/Result.h>
#include <core/StringView.h>

#include <sqlite3.h>

namespace ledger
{
	class Ledger
	{
		core::Allocator* m_allocator = nullptr;
		core::String m_path;
		sqlite3* m_db = nullptr;
		sqlite3_stmt* m_getAppID = nullptr;
		sqlite3_stmt* m_getUserVersion = nullptr;

		void destroy()
		{
			if (m_getAppID)
			{
				sqlite3_finalize(m_getAppID);
				m_getAppID = nullptr;
			}

			if (m_getUserVersion)
			{
				sqlite3_finalize(m_getUserVersion);
				m_getUserVersion = nullptr;
			}

			if (m_db)
			{
				sqlite3_close(m_db);
				m_db = nullptr;
			}
		}

		void moveFrom(Ledger&& other)
		{
			m_allocator = other.m_allocator;
			m_path = std::move(other.m_path);
			m_db = other.m_db;

			other.m_allocator = nullptr;
			other.m_db = nullptr;
		}

		Ledger(sqlite3* db, core::String path, core::Allocator* allocator);

		core::Result<sqlite3_stmt*> createStmt(core::StringView query);

	public:
		struct Version
		{
			int32_t appID;
			int32_t userVersion;
		};

		LEDGER_EXPORT static core::Result<Ledger> open(core::StringView path, core::Allocator* allocator);

		Ledger(const Ledger&) = delete;
		Ledger& operator=(const Ledger&) = delete;

		Ledger(Ledger&& other) noexcept
			: m_path(other.m_allocator)
		{
			moveFrom(std::move(other));
		}

		Ledger& operator=(Ledger&& other) noexcept
		{
			destroy();
			moveFrom(std::move(other));
			return *this;
		}

		~Ledger() noexcept
		{
			destroy();
		}

		LEDGER_EXPORT core::Result<Version> queryVersion();
	};
}
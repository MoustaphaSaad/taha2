#include "minijava/Unit.h"
#include "minijava/Scanner.h"

#include <core/File.h>
#include <core/Path.h>

namespace minijava
{
	core::Result<core::Unique<Unit>> Unit::create(core::StringView path, core::Allocator* allocator)
	{
		auto absolutePathResult = core::Path::abs(path, allocator);
		if (absolutePathResult.isError())
			return absolutePathResult.releaseError();
		auto absolutePath = absolutePathResult.releaseValue();

		auto contentResult = core::File::content(allocator, absolutePath);
		if (contentResult.isError())
			return contentResult.releaseError();
		auto content = contentResult.releaseValue();

		auto filePath = core::String{path, allocator};

		return core::unique_from<Unit>(allocator, std::move(filePath), std::move(absolutePath), std::move(content), allocator);
	}

	bool Unit::scan()
	{
		if (m_stage >= STAGE_SCANNED)
			return m_stage != STAGE_FAILED;

		Scanner scanner{this, m_allocator};
		while (true)
		{
			auto token = scanner.scan();

			if (token.kind() == Token::KIND_EOF)
				break;

			if (token.kind() != Token::KIND_NONE)
				m_token.push(std::move(token));
		}

		if (hasErrors())
		{
			m_stage = STAGE_FAILED;
			return false;
		}
		else
		{
			m_stage = STAGE_SCANNED;
			return true;
		}
	}

	void Unit::dumpTokens(core::Stream* stream)
	{
		for (const auto& token: m_token)
		{
			core::strf(stream, "Token: \"{}\", Type: \"{}\", Line: {}:{}\n"_sv,
				token.text(),
				token.kind(),
				token.location().position.line,
				token.location().position.column
			);
		}
	}

	void Unit::dumpErrors(core::Stream* stream)
	{
		for (size_t i = 0; i < m_errors.count(); ++i)
		{
			const auto& err = m_errors[i];

			if (i > 0)
				core::strf(stream, "\n"_sv);

			if (err.location().position.line > 0)
			{
				if (err.location().range.count() > 0)
				{
					auto l = m_lines[err.location().position.line - 1];
					core::strf(stream, ">> {}\n"_sv, l);
					core::strf(stream, ">> "_sv);
					for (auto it = l.begin(); it != l.end(); it = core::Rune::next(it))
					{
						auto r = core::Rune::decode(it);

						if (r == '\r' || r == '\n')
						{

						}
						else if (it >= err.location().range.begin() && it < err.location().range.end())
						{
							core::strf(stream, "^"_sv);
						}
						else if (r == '\t')
						{
							core::strf(stream, "\t"_sv);
						}
						else
						{
							core::strf(stream, " "_sv);
						}
					}
					core::strf(stream, "\n"_sv);
				}

				core::strf(
					stream,
					"Error[{}:{}:{}]: {}"_sv,
					err.location().unit->m_filePath,
					err.location().position.line,
					err.location().position.column,
					err.message()
				);
			}
			else
			{
				if (err.location().unit != nullptr)
				{
					core::strf(stream, "Error[%s]: %s"_sv, err.location().unit->m_filePath, err.message());
				}
				else
				{
					core::strf(stream, "Error: %s"_sv, err.message());
				}
			}
		}
	}
}
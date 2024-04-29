#pragma once

#include <math/Vec.h>

namespace taha
{
	class Command
	{
	public:
		enum KIND
		{
			KIND_NONE,
			KIND_CLEAR,
		};

		Command(KIND kind)
			: m_kind(kind)
		{}
		virtual ~Command() = default;

		KIND kind() const { return m_kind; }
	private:
		KIND m_kind = KIND_NONE;
	};

	class ClearCommand: public Command
	{
	public:
		math::rgba color = {0, 0, 0, 1};
		float depth = 1.0f;
		uint8_t stencil = 0;

		ClearCommand()
			: Command(Command::KIND_CLEAR)
		{}
	};
}
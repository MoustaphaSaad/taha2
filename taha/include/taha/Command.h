#pragma once

#include <math/Vec.h>

namespace taha
{
	constexpr size_t CONSTANT_MAX_COLOR_ATTACHMENT_SIZE = 4;

	enum class Action
	{
		Clear,
		Load,
		DontCare,
	};

	struct FrameColorAction
	{
		Action action = Action::Clear;
		math::rgba value = {0, 0, 0, 1};
	};

	struct FrameDepthAction
	{
		Action action = Action::Clear;
		float value = 1.0f;
	};

	struct FrameStencilAction
	{
		Action action = Action::Clear;
		uint8_t value = 0;
	};

	struct FrameAction
	{
		bool independentClearColor = false;
		FrameColorAction color[CONSTANT_MAX_COLOR_ATTACHMENT_SIZE];
		FrameDepthAction depth;
		FrameStencilAction stencil;
	};

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
		FrameAction action;

		ClearCommand()
			: Command(Command::KIND_CLEAR)
		{}
	};
}
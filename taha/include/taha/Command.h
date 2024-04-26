#pragma once

namespace taha
{
	class Command
	{
	public:
		virtual ~Command() = default;
	};

	class ClearCommand: public Command
	{
	public:
	};
}
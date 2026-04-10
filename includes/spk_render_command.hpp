#pragma once

namespace spk
{
	class RenderCommand
	{
	public:
		virtual ~RenderCommand() = default;

		virtual void execute() const = 0;
	};
}
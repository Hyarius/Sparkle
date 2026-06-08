#pragma once

namespace spk
{
	class RenderContext;

	class RenderCommand
	{
	public:
		virtual ~RenderCommand() = default;

		virtual void execute(spk::RenderContext& p_renderContext) = 0;
	};
}

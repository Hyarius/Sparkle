#pragma once

namespace spk
{
	class IRenderContext;

	class RenderCommand
	{
	public:
		virtual ~RenderCommand() = default;

		virtual void execute(spk::IRenderContext& p_renderContext) = 0;
	};
}

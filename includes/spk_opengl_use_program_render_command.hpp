#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>

#include "spk_program.hpp"
#include "spk_render_command.hpp"

namespace spk::OpenGL
{
	class UseProgramRenderCommand : public spk::RenderCommand
	{
	private:
		std::shared_ptr<spk::Program> _program;

	public:
		explicit UseProgramRenderCommand(std::shared_ptr<spk::Program> p_program = nullptr);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif

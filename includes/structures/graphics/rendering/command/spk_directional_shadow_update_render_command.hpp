#pragma once
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include <array>
#include <memory>
namespace spk
{
	class FrameBufferObject;
	class UniformBufferObject;
	class DirectionalShadowUpdateRenderCommand final : public RenderCommand
	{
		std::shared_ptr<const UniformBufferObject> _data;
		std::array<std::shared_ptr<const FrameBufferObject>, 4> _targets;

	public:
		DirectionalShadowUpdateRenderCommand(std::shared_ptr<const UniformBufferObject>, std::array<std::shared_ptr<const FrameBufferObject>, 4>);
		void execute(RenderContext &) override;
	};
}

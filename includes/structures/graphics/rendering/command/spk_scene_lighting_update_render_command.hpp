#pragma once

#include <memory>

#include "structures/graphics/rendering/command/spk_render_command.hpp"

namespace spk
{
	class UniformBufferObject;
	class ShaderStorageBufferObject;

	// Retains a published, immutable set of GPU buffers for one render frame.
	class SceneLightingUpdateRenderCommand final : public spk::RenderCommand
	{
	private:
		std::shared_ptr<const spk::UniformBufferObject> _header;
		std::shared_ptr<const spk::ShaderStorageBufferObject> _directional;
		std::shared_ptr<const spk::ShaderStorageBufferObject> _point;
		std::shared_ptr<const spk::ShaderStorageBufferObject> _spot;

	public:
		SceneLightingUpdateRenderCommand(
			std::shared_ptr<const spk::UniformBufferObject> p_header,
			std::shared_ptr<const spk::ShaderStorageBufferObject> p_directional,
			std::shared_ptr<const spk::ShaderStorageBufferObject> p_point,
			std::shared_ptr<const spk::ShaderStorageBufferObject> p_spot);
		void execute(spk::RenderContext &p_renderContext) override;
	};
}

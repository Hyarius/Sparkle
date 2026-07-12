#include "structures/graphics/rendering/command/spk_scene_lighting_update_render_command.hpp"

#include <stdexcept>

#include "structures/graphics/spk_shader_storage_buffer_object.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

namespace spk
{
	SceneLightingUpdateRenderCommand::SceneLightingUpdateRenderCommand(
		std::shared_ptr<const spk::UniformBufferObject> p_header,
		std::shared_ptr<const spk::ShaderStorageBufferObject> p_directional,
		std::shared_ptr<const spk::ShaderStorageBufferObject> p_point,
		std::shared_ptr<const spk::ShaderStorageBufferObject> p_spot) :
		_header(std::move(p_header)), _directional(std::move(p_directional)), _point(std::move(p_point)), _spot(std::move(p_spot))
	{
		if (_header == nullptr || _directional == nullptr || _point == nullptr || _spot == nullptr)
			throw std::invalid_argument("Scene lighting command requires every published buffer");
	}

	void SceneLightingUpdateRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		_header->activate(p_renderContext);
		_directional->activate(p_renderContext);
		_point->activate(p_renderContext);
		_spot->activate(p_renderContext);
	}
}

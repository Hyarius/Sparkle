#include "rendering/light_update_render_command.hpp"

#include "structures/graphics/spk_buffer_object.hpp"

namespace pg
{
	LightUpdateRenderCommand::LightUpdateRenderCommand(
		const spk::Vector3 &p_direction,
		const spk::Color &p_color,
		float p_ambient) :
		_data{
			.direction = {p_direction.x, p_direction.y, p_direction.z, 0.0f},
			.color = p_color.values(),
			.ambient = {p_ambient, 0.0f, 0.0f, 0.0f}},
		_lightUBO(LightBinding, spk::BufferObject::Usage::DynamicDraw, sizeof(LightData))
	{
	}

	void LightUpdateRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		_lightUBO.edit(&_data, sizeof(_data));
		_lightUBO.activate(p_renderContext);
	}
}

#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"

#include "structures/graphics/spk_buffer_object.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
	[[nodiscard]] GLuint checkedBindingPoint(std::size_t p_bindingPoint)
	{
		if (p_bindingPoint > std::numeric_limits<GLuint>::max())
		{
			throw std::out_of_range("spk::DirectionalLightUpdateRenderCommand: binding point exceeds GLuint range");
		}
		return static_cast<GLuint>(p_bindingPoint);
	}
}

namespace spk
{
	DirectionalLightUpdateRenderCommand::DirectionalLightUpdateRenderCommand(
		std::size_t p_bindingPoint,
		spk::DirectionalLight p_light) :
		_light(std::move(p_light)),
		_lightUBO(
			checkedBindingPoint(p_bindingPoint),
			spk::BufferObject::Usage::DynamicDraw,
			sizeof(spk::DirectionalLight))
	{
	}

	void DirectionalLightUpdateRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		_lightUBO.edit(&_light, sizeof(_light));
		_lightUBO.activate(p_renderContext);
	}
}

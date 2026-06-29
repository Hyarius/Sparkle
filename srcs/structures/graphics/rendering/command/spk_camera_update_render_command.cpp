#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"

#include "structures/graphics/spk_buffer_object.hpp"

namespace spk
{
	CameraUpdateRenderCommand::CameraUpdateRenderCommand(GLuint p_bindingPoint, const spk::Matrix4x4 &p_viewProjection) :
		_viewProjection(p_viewProjection),
		_cameraUBO(p_bindingPoint, spk::BufferObject::Usage::DynamicDraw, sizeof(spk::Matrix4x4))
	{
	}

	void CameraUpdateRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		_cameraUBO.edit(&_viewProjection, sizeof(_viewProjection));
		_cameraUBO.activate(p_renderContext);
	}
}

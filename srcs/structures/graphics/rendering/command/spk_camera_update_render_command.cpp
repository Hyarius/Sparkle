#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"

#include "structures/graphics/spk_buffer_object.hpp"

namespace spk
{
	CameraUpdateRenderCommand::CameraUpdateRenderCommand(GLuint p_bindingPoint, const spk::Matrix4x4 &p_viewProjection) :
		CameraUpdateRenderCommand(p_bindingPoint, p_viewProjection, spk::Matrix4x4::identity())
	{
	}

	CameraUpdateRenderCommand::CameraUpdateRenderCommand(GLuint p_bindingPoint, const spk::Matrix4x4 &p_viewProjection, const spk::Matrix4x4 &p_view) :
		_camera{.viewProjection = p_viewProjection, .view = p_view},
		_cameraUBO(p_bindingPoint, spk::BufferObject::Usage::DynamicDraw, sizeof(spk::CameraGpuData))
	{
	}

	void CameraUpdateRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		_cameraUBO.edit(&_camera, sizeof(_camera));
		_cameraUBO.activate(p_renderContext);
	}
}

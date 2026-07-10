#include "structures/graphics/rendering/command/spk_draw_voxel_mesh_3d_render_command.hpp"

#include <utility>

#include "spk_generated_resources.hpp"
#include "structures/graphics/rendering/command/spk_opengl_draw_state_guard.hpp"
#include "structures/graphics/spk_buffer_object.hpp"
#include "structures/graphics/spk_primitive.hpp"
#include "structures/graphics/spk_uniform.hpp"

namespace spk
{
	spk::Program &DrawVoxelMesh3DRenderCommand::_program()
	{
		static spk::Program program(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/mesh_3d/draw_voxel_mesh_3d.vert"),
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/mesh_3d/draw_voxel_mesh_3d.frag"));
		return program;
	}

	std::shared_ptr<spk::UniformBufferObject> DrawVoxelMesh3DRenderCommand::makeModelUBO(
		const spk::Matrix4x4 &p_model,
		const spk::Color &p_tint)
	{
		auto modelUBO = std::make_shared<spk::UniformBufferObject>(
			ModelBinding, spk::BufferObject::Usage::DynamicDraw, sizeof(ModelData));
		updateModelUBO(*modelUBO, p_model, p_tint);
		return modelUBO;
	}

	void DrawVoxelMesh3DRenderCommand::updateModelUBO(
		spk::UniformBufferObject &p_modelUBO,
		const spk::Matrix4x4 &p_model,
		const spk::Color &p_tint)
	{
		const ModelData data{.model = p_model, .tint = p_tint};
		p_modelUBO.edit(&data, sizeof(data));
	}

	std::shared_ptr<spk::SamplerObject> DrawVoxelMesh3DRenderCommand::makeSampler(const spk::Texture &p_texture)
	{
		auto sampler = std::make_shared<spk::SamplerObject>(
			"uTexture", spk::SamplerObject::Type::Texture2D, 0, _program());
		sampler->bind(p_texture);
		return sampler;
	}

	DrawVoxelMesh3DRenderCommand::DrawVoxelMesh3DRenderCommand(
		std::shared_ptr<const spk::VoxelMesh3D> p_mesh,
		std::shared_ptr<spk::UniformBufferObject> p_modelUBO,
		std::shared_ptr<spk::SamplerObject> p_sampler,
		bool p_translucent) :
		_mesh(std::move(p_mesh)),
		_modelUBO(std::move(p_modelUBO)),
		_sampler(std::move(p_sampler)),
		_translucent(p_translucent)
	{
	}

	void DrawVoxelMesh3DRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		if (_mesh == nullptr || _modelUBO == nullptr || _sampler == nullptr ||
			_mesh->layoutBuffer().indexCount() == 0)
		{
			return;
		}

		spk::OpenGLDrawStateGuard stateGuard;
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(_translucent ? GL_FALSE : GL_TRUE);
		if (_translucent)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glDisable(GL_BLEND);
		}

		spk::Program &program = _program();
		program.activate(p_renderContext);
		_modelUBO->activate(p_renderContext);
		static spk::BoolUniform translucentUniform("uTranslucent", program);
		translucentUniform.set(_translucent);
		translucentUniform.activate();
		_sampler->activate(p_renderContext);
		_mesh->layoutBuffer().activate(p_renderContext);
		program.render(p_renderContext, spk::Primitive::Triangles, 0, _mesh->layoutBuffer().indexCount());
	}
}

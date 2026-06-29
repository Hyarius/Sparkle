#include "structures/graphics/rendering/command/spk_instanced_sprite_render_command.hpp"

#include "spk_generated_resources.hpp"
#include "structures/graphics/spk_buffer_object.hpp"
#include "structures/graphics/spk_primitive.hpp"

namespace spk
{
	spk::Program &InstancedSpriteRenderCommand::_program()
	{
		static spk::Program program(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/sprite_batch/sprite_batch.vert"),
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/sprite_batch/sprite_batch.frag"));
		return program;
	}

	InstancedSpriteRenderCommand::Instance::Instance(GPUInstanceData &p_data) :
		_data(&p_data)
	{
	}

	void InstancedSpriteRenderCommand::Instance::setModelTransform(const spk::Matrix4x4 &p_modelTransform)
	{
		_data->modelTransform = p_modelTransform;
	}

	void InstancedSpriteRenderCommand::Instance::setSprite(const spk::SpriteSheet::Sprite &p_sprite)
	{
		_data->sprite = p_sprite;
	}

	InstancedSpriteRenderCommand::InstancedSpriteRenderCommand(
		const spk::SpriteSheet &p_spriteSheet,
		const spk::TextureMesh2D &p_mesh) :
		_spriteSheet(p_spriteSheet),
		_mesh(p_mesh),
		_instanceSSBO(InstanceBinding, spk::BufferObject::Usage::StreamDraw, 0, sizeof(GPUInstanceData)),
		_sampler("uTexture", spk::SamplerObject::Type::Texture2D, 0, _program())
	{
		_sampler.bind(_spriteSheet);
	}

	InstancedSpriteRenderCommand::Instance InstancedSpriteRenderCommand::pushBackInstance()
	{
		return Instance(_instanceSSBO.emplaceBack<GPUInstanceData>());
	}

	std::size_t InstancedSpriteRenderCommand::instanceCount() const
	{
		return _instanceSSBO.count();
	}

	const spk::SpriteSheet &InstancedSpriteRenderCommand::spriteSheet() const
	{
		return _spriteSheet;
	}

	const spk::TextureMesh2D &InstancedSpriteRenderCommand::mesh() const
	{
		return _mesh;
	}

	void InstancedSpriteRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		const std::size_t instanceCount = _instanceSSBO.count();
		if (instanceCount == 0)
		{
			return;
		}

		spk::Program &program = _program();
		program.activate(p_renderContext);
		_instanceSSBO.activate(p_renderContext);
		_sampler.activate(p_renderContext);
		_mesh.layoutBuffer().activate(p_renderContext);
		program.renderInstanced(
			p_renderContext,
			spk::Primitive::Triangles,
			0,
			_mesh.layoutBuffer().indexCount(),
			instanceCount);
	}
}

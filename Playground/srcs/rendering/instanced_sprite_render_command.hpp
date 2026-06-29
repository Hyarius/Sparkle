#pragma once

#include <cstddef>
#include <type_traits>

#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_shader_storage_buffer_object.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/math/spk_matrix.hpp"

namespace pg
{
	class InstancedSpriteRenderCommand : public spk::RenderCommand
	{
	private:
		struct GPUInstanceData;

	public:
		class Instance
		{
			friend class InstancedSpriteRenderCommand;

		private:
			GPUInstanceData *_data = nullptr;

			explicit Instance(GPUInstanceData &p_data);

		public:
			void setModelTransform(const spk::Matrix4x4 &p_modelTransform);
			void setSprite(const spk::SpriteSheet::Sprite &p_sprite);
		};

		InstancedSpriteRenderCommand(
			const spk::SpriteSheet &p_spriteSheet,
			const spk::TextureMesh2D &p_mesh);

		[[nodiscard]] Instance pushBackInstance();
		[[nodiscard]] std::size_t instanceCount() const;
		[[nodiscard]] const spk::SpriteSheet &spriteSheet() const;
		[[nodiscard]] const spk::TextureMesh2D &mesh() const;

		void execute(spk::RenderContext &p_renderContext) override;

	private:
		static constexpr GLuint InstanceBinding = 0;

		struct GPUInstanceData
		{
			spk::Matrix4x4 modelTransform = spk::Matrix4x4::identity();
			spk::SpriteSheet::Sprite sprite = spk::SpriteSheet::Sprite::whole;
		};

		static_assert(std::is_standard_layout_v<GPUInstanceData>);
		static_assert(std::is_trivially_copyable_v<GPUInstanceData>);
		static_assert(sizeof(spk::Matrix4x4) == 64);
		static_assert(sizeof(GPUInstanceData) == 80);
		static_assert(offsetof(GPUInstanceData, modelTransform) == 0);
		static_assert(offsetof(GPUInstanceData, sprite) == 64);

		const spk::SpriteSheet &_spriteSheet;
		const spk::TextureMesh2D &_mesh;
		spk::ShaderStorageBufferObject _instanceSSBO;
		spk::SamplerObject _sampler;

		[[nodiscard]] static spk::Program &_program();
	};
}

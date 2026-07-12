#pragma once

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/voxel/spk_voxel_chunk_renderer.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace spk
{
	class Camera3D;
	class SamplerObject;
	class Texture;
	class UniformBufferObject;

	// Transparent chunk contributor. The SceneTransparent phase structurally follows
	// every opaque contributor; chunks are sorted back-to-front within this contributor.
	class VoxelChunkTransparentRenderLogic : public spk::ComponentLogic<spk::VoxelChunkRenderer>
	{
	private:
		struct CachedDraw
		{
			bool hasModel = false;
			spk::Matrix4x4 model;
			std::shared_ptr<spk::UniformBufferObject> modelUBO;
		};

		const spk::Texture &_texture;
		std::shared_ptr<spk::SamplerObject> _sampler;
		spk::Camera3D *_camera = nullptr;
		std::unordered_map<const spk::VoxelChunkRenderer *, CachedDraw> _cache;
		std::vector<const spk::VoxelChunkRenderer *> _visibleChunks;

		void _syncCache(CachedDraw &p_cached, const spk::Matrix4x4 &p_model);
		void _pruneUnloadedChunks();

	public:
		explicit VoxelChunkTransparentRenderLogic(const spk::Texture &p_texture);
		[[nodiscard]] spk::RenderPhaseMask renderPhases() const noexcept override
		{
			return spk::renderPhaseBit(spk::RenderPhase::SceneTransparent);
		}

	protected:
		void _onRenderStarted(std::size_t p_componentCount) override;
		void _parseComponentForRender(spk::VoxelChunkRenderer &p_renderer) override;
		void _executeRender(spk::RenderUnitBuilder &p_builder) override;
		void _onRenderPhaseStarted(const spk::RenderPhaseContext &p_context, std::size_t p_componentCount) override;
		void _parseComponentForRender(
			const spk::RenderPhaseContext &p_context,
			spk::VoxelChunkRenderer &p_renderer) override
		{
			(void)p_context;
			_parseComponentForRender(p_renderer);
		}
		void _executeRender(const spk::RenderPhaseContext &p_context, spk::RenderPass &p_pass) override
		{
			spk::RenderUnitBuilder builder;
			_executeRender(builder);
			spk::RenderUnit unit = builder.build();
			for (auto &command : unit.takeCommands())
			{
				p_pass.add(p_context.phase, std::move(command));
			}
		}
	};
}

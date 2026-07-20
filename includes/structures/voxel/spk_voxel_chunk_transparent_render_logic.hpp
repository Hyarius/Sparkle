#pragma once

#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_pipeline.hpp"
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

	protected:
		void _onRenderStarted(const spk::SceneRenderBuildContext &p_context, std::size_t p_componentCount) override;
		void _parseComponentForRender(
			const spk::SceneRenderBuildContext &p_context,
			spk::VoxelChunkRenderer &p_renderer) override;
		void _executeRender(const spk::SceneRenderBuildContext &p_context) override;
	};
}

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

	// Late render pass for transparent chunk geometry. Its lower default priority places
	// it after the normal-priority opaque voxel and actor render logics. Chunks are then
	// ordered back-to-front within this pass for alpha blending.
	class VoxelChunkTransparentRenderLogic : public spk::ComponentLogic<spk::VoxelChunkRenderer>
	{
	public:
		static constexpr spk::PriorizableTrait::Priority DefaultPriority = -100;

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
		void _onRenderStarted(std::size_t p_componentCount) override;
		void _parseComponentForRender(spk::VoxelChunkRenderer &p_renderer) override;
		void _executeRender(spk::RenderUnitBuilder &p_builder) override;
	};
}

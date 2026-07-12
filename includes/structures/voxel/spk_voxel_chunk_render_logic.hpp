#pragma once

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass_bucket_pack.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/system/spk_profiler.hpp"
#include "structures/system/thread/spk_worker_pool.hpp"
#include "structures/voxel/spk_voxel_chunk_renderer.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace spk
{
	class Camera3D;
	class SamplerObject;
	class Texture;
	class UniformBufferObject;

	// Synchronizes dirty chunk meshes and submits their opaque geometry. Transparent
	// geometry is intentionally handled by VoxelChunkTransparentRenderLogic so every
	// opaque scene object can populate the depth buffer first.
	class VoxelChunkRenderLogic : public spk::ComponentLogic<spk::VoxelChunkRenderer>
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
		spk::WorkerPool *_workerPool = nullptr;

		// Timing probes, resolved from the profiler carried by the UpdateContext (owned by
		// the application). "batch" spans the whole worker dispatch + join for the chunks
		// dirtied this frame; "chunk" is the per-chunk mesh build, recorded from the worker
		// threads; "assembly" is the render-command building (which also runs on the update
		// thread, so it reuses the same cached profiler). Null until a profiler is bound.
		spk::Profiler *_profiler = nullptr;
		spk::Profiler::Probe *_bakeBatchProbe = nullptr;
		spk::Profiler::Probe *_bakeChunkProbe = nullptr;
		spk::Profiler::Probe *_renderAssemblyProbe = nullptr;

		void _bindProfiler(spk::Profiler *p_profiler);

		spk::Camera3D *_camera = nullptr;
		spk::Matrix4x4 _cameraMatrix;

		std::vector<spk::VoxelChunkRenderer *> _dirtyChunks;
		std::unordered_map<const spk::VoxelChunkRenderer *, CachedDraw> _cache;
		std::vector<const spk::VoxelChunkRenderer *> _visibleChunks;

		void _syncCache(CachedDraw &p_cached, const spk::Matrix4x4 &p_model);
		void _pruneUnloadedChunks();
		void _synchronizeDirtyChunks();

	public:
		// The referenced pool must outlive this logic.
		explicit VoxelChunkRenderLogic(
			const spk::Texture &p_texture,
			spk::WorkerPool &p_workerPool = spk::WorkerPool::global());

		[[nodiscard]] spk::WorkerPool &workerPool() noexcept;
		[[nodiscard]] const spk::WorkerPool &workerPool() const noexcept;
	protected:
		void _onUpdateStarted(const spk::UpdateContext &p_tick) override;
		void _parseComponentForUpdate(const spk::UpdateContext &p_tick, spk::VoxelChunkRenderer &p_renderer) override;
		void _executeUpdate(const spk::UpdateContext &p_tick) override;

		void _onRenderStarted(const spk::SceneRenderBuildContext &p_context, std::size_t p_componentCount) override;
		void _parseComponentForRender(
			const spk::SceneRenderBuildContext &p_context,
			spk::VoxelChunkRenderer &p_renderer) override;
		void _executeRender(const spk::SceneRenderBuildContext &p_context) override;
	};
}

#pragma once

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/math/spk_matrix.hpp"
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

	class VoxelChunkRenderLogic : public spk::ComponentLogic<spk::VoxelChunkRenderer>
	{
	private:
		static constexpr unsigned int CameraBinding = 1;
		static constexpr unsigned int DirectionalLightBinding = 3;
		static constexpr float AmbientLight = 0.35f;

		struct CachedDraw
		{
			bool hasModel = false;
			spk::Matrix4x4 model;
			std::shared_ptr<spk::UniformBufferObject> modelUBO;
		};

		const spk::Texture &_texture;
		std::shared_ptr<spk::SamplerObject> _sampler;
		bool _emitFrameState = true;
		spk::WorkerPool *_workerPool = nullptr;

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
			bool p_emitFrameState = true,
			spk::WorkerPool &p_workerPool = spk::WorkerPool::global());

		[[nodiscard]] spk::WorkerPool &workerPool() noexcept;
		[[nodiscard]] const spk::WorkerPool &workerPool() const noexcept;

	protected:
		void _onUpdateStarted(const spk::UpdateTick &p_tick) override;
		void _parseComponentForUpdate(const spk::UpdateTick &p_tick, spk::VoxelChunkRenderer &p_renderer) override;
		void _executeUpdate(const spk::UpdateTick &p_tick) override;

		void _onRenderStarted(std::size_t p_componentCount) override;
		void _parseComponentForRender(spk::VoxelChunkRenderer &p_renderer) override;
		void _executeRender(spk::RenderUnitBuilder &p_builder) override;
	};
}

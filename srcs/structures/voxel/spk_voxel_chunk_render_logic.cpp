#include "structures/voxel/spk_voxel_chunk_render_logic.hpp"

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_transform_3d.hpp"
#include "structures/graphics/rendering/command/spk_draw_voxel_mesh_3d_render_command.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

#include <algorithm>
#include <cstring>
#include <exception>
#include <iterator>
#include <optional>
#include <unordered_set>
#include <utility>

namespace spk
{
	VoxelChunkRenderLogic::VoxelChunkRenderLogic(
		const spk::Texture &p_texture,
		spk::WorkerPool &p_workerPool) :
		_texture(p_texture),
		_workerPool(&p_workerPool)
	{
	}

	spk::WorkerPool &VoxelChunkRenderLogic::workerPool() noexcept
	{
		return *_workerPool;
	}

	const spk::WorkerPool &VoxelChunkRenderLogic::workerPool() const noexcept
	{
		return *_workerPool;
	}

	void VoxelChunkRenderLogic::_bindProfiler(spk::Profiler *p_profiler)
	{
		if (p_profiler == _profiler)
		{
			return;
		}

		_profiler = p_profiler;
		if (_profiler == nullptr)
		{
			_bakeBatchProbe = nullptr;
			_bakeChunkProbe = nullptr;
			_renderAssemblyProbe = nullptr;
			return;
		}

		_bakeBatchProbe = &_profiler->probe("Voxel/Bake (batch)");
		_bakeChunkProbe = &_profiler->probe("Voxel/Bake (chunk)");
		_renderAssemblyProbe = &_profiler->probe("Voxel/Render assembly");
	}

	void VoxelChunkRenderLogic::_syncCache(CachedDraw &p_cached, const spk::Matrix4x4 &p_model)
	{
		if (p_cached.modelUBO == nullptr)
		{
			p_cached.modelUBO = spk::DrawVoxelMesh3DRenderCommand::makeModelUBO(p_model, spk::Color{1, 1, 1, 1});
			p_cached.model = p_model;
			p_cached.hasModel = true;
		}
		else if (!p_cached.hasModel || std::memcmp(&p_cached.model, &p_model, sizeof(p_model)) != 0)
		{
			spk::DrawVoxelMesh3DRenderCommand::updateModelUBO(*p_cached.modelUBO, p_model, spk::Color{1, 1, 1, 1});
			p_cached.model = p_model;
			p_cached.hasModel = true;
		}
	}

	void VoxelChunkRenderLogic::_pruneUnloadedChunks()
	{
		if (_cache.size() == _visibleChunks.size())
		{
			return;
		}
		const std::unordered_set<const spk::VoxelChunkRenderer *> visible(_visibleChunks.begin(), _visibleChunks.end());
		for (auto iterator = _cache.begin(); iterator != _cache.end();)
		{
			iterator = visible.contains(iterator->first) ? std::next(iterator) : _cache.erase(iterator);
		}
	}

	void VoxelChunkRenderLogic::_synchronizeDirtyChunks()
	{
		if (_dirtyChunks.empty())
		{
			return;
		}

		// Only timed when there is real work, so the min/avg stay meaningful.
		std::optional<spk::Profiler::Probe::Sample> batchSample;
		if (_bakeBatchProbe != nullptr)
		{
			batchSample.emplace(*_bakeBatchProbe);
		}

		struct PendingMesh
		{
			spk::VoxelChunkRenderer *renderer = nullptr;
			spk::WorkerPool::Task<spk::VoxelChunkRenderer::MeshHandle> task;
		};

		std::vector<PendingMesh> pendingMeshes;
		pendingMeshes.reserve(_dirtyChunks.size());
		std::exception_ptr firstFailure;
		for (spk::VoxelChunkRenderer *renderer : _dirtyChunks)
		{
			if (renderer->beginMeshSynchronization() == false)
			{
				continue;
			}
			try
			{
				pendingMeshes.push_back(PendingMesh{.renderer = renderer, .task = _workerPool->push([renderer, probe = _bakeChunkProbe]() {
														std::optional<spk::Profiler::Probe::Sample> chunkSample;
														if (probe != nullptr)
														{
															chunkSample.emplace(*probe);
														}
														return renderer->buildMesh();
													})});
			} catch (...)
			{
				renderer->failMeshSynchronization();
				firstFailure = std::current_exception();
				break;
			}
		}
		_dirtyChunks.clear();

		for (PendingMesh &pending : pendingMeshes)
		{
			try
			{
				pending.renderer->publishMesh(pending.task.obtain());
			} catch (...)
			{
				pending.renderer->failMeshSynchronization();
				if (firstFailure == nullptr)
				{
					firstFailure = std::current_exception();
				}
			}
		}

		if (firstFailure != nullptr)
		{
			std::rethrow_exception(firstFailure);
		}
	}

	void VoxelChunkRenderLogic::_onUpdateStarted(const spk::UpdateContext &p_tick)
	{
		// The render-command assembly step also runs on the update thread (right after this
		// pass), so caching the profiler here covers both the bake and the assembly probes.
		_bindProfiler(p_tick.profiler);
		_dirtyChunks.clear();
	}

	void VoxelChunkRenderLogic::_parseComponentForUpdate(
		const spk::UpdateContext &p_tick,
		spk::VoxelChunkRenderer &p_renderer)
	{
		(void)p_tick;
		if (p_renderer.needsSynchronization())
		{
			_dirtyChunks.push_back(&p_renderer);
		}
	}

	void VoxelChunkRenderLogic::_executeUpdate(const spk::UpdateContext &p_tick)
	{
		(void)p_tick;
		_synchronizeDirtyChunks();
	}

	void VoxelChunkRenderLogic::_onRenderStarted(std::size_t p_componentCount)
	{
		_visibleChunks.clear();
		_visibleChunks.reserve(p_componentCount);
		_camera = spk::Camera3D::mainCamera();
		if (_camera != nullptr)
		{
			_cameraMatrix = _camera->viewProjectionMatrix();
		}
		if (_sampler == nullptr)
		{
			_sampler = spk::DrawVoxelMesh3DRenderCommand::makeSampler(_texture);
		}
	}

	void VoxelChunkRenderLogic::_onRenderPhaseStarted(
		const spk::RenderPhaseContext &p_context,
		std::size_t p_componentCount)
	{
		_onRenderStarted(p_componentCount);
		_camera = p_context.frame.mainCamera;
		if (_camera != nullptr)
		{
			_cameraMatrix = _camera->viewProjectionMatrix();
		}
	}

	void VoxelChunkRenderLogic::_parseComponentForRender(spk::VoxelChunkRenderer &p_renderer)
	{
		if (_camera == nullptr || p_renderer.needsSynchronization())
		{
			return;
		}

		spk::Matrix4x4 model = spk::Matrix4x4::identity();
		if (const spk::Entity *owner = p_renderer.entity(); owner != nullptr)
		{
			if (const spk::Transform3D *transform = owner->component<spk::Transform3D>(); transform != nullptr)
			{
				model = transform->modelTransform();
			}
		}

		_syncCache(_cache[&p_renderer], model);
		_visibleChunks.push_back(&p_renderer);
	}

	void VoxelChunkRenderLogic::_executeRender(spk::RenderUnitBuilder &p_builder)
	{
		const bool hasRenderableMesh = std::ranges::any_of(_visibleChunks, [](const auto *p_renderer) {
			const spk::VoxelRenderMeshes &meshes = p_renderer->meshes();
			return meshes.opaque.indexes().empty() == false || meshes.transparent.indexes().empty() == false;
		});

		if (_camera == nullptr || !hasRenderableMesh)
		{
			_pruneUnloadedChunks();
			return;
		}

		std::optional<spk::Profiler::Probe::Sample> assemblySample;
		if (_renderAssemblyProbe != nullptr)
		{
			assemblySample.emplace(*_renderAssemblyProbe);
		}

		for (const spk::VoxelChunkRenderer *renderer : _visibleChunks)
		{
			if (renderer->meshes().opaque.indexes().empty())
			{
				continue;
			}
			p_builder.add(std::make_unique<spk::DrawVoxelMesh3DRenderCommand>(renderer->sharedOpaqueMesh(), _cache.at(renderer).modelUBO, _sampler, false));
		}

		_pruneUnloadedChunks();
	}
}

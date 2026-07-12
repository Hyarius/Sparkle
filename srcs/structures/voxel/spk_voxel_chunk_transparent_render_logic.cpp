#include "structures/voxel/spk_voxel_chunk_transparent_render_logic.hpp"

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_transform_3d.hpp"
#include "structures/graphics/rendering/command/spk_draw_voxel_mesh_3d_render_command.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <unordered_set>
#include <utility>

namespace spk
{
	VoxelChunkTransparentRenderLogic::VoxelChunkTransparentRenderLogic(const spk::Texture &p_texture) :
		_texture(p_texture)
	{
	}

	void VoxelChunkTransparentRenderLogic::_onRenderPhaseStarted(
		const spk::RenderPhaseContext &p_context,
		std::size_t p_componentCount)
	{
		_onRenderStarted(p_componentCount);
		_camera = p_context.frame.mainCamera;
	}

	void VoxelChunkTransparentRenderLogic::_syncCache(CachedDraw &p_cached, const spk::Matrix4x4 &p_model)
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

	void VoxelChunkTransparentRenderLogic::_pruneUnloadedChunks()
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

	void VoxelChunkTransparentRenderLogic::_onRenderStarted(std::size_t p_componentCount)
	{
		_visibleChunks.clear();
		_visibleChunks.reserve(p_componentCount);
		_camera = spk::Camera3D::mainCamera();
		if (_sampler == nullptr)
		{
			_sampler = spk::DrawVoxelMesh3DRenderCommand::makeSampler(_texture);
		}
	}

	void VoxelChunkTransparentRenderLogic::_parseComponentForRender(spk::VoxelChunkRenderer &p_renderer)
	{
		if (_camera == nullptr || p_renderer.needsSynchronization() || p_renderer.meshes().transparent.indexes().empty())
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

	void VoxelChunkTransparentRenderLogic::_executeRender(spk::RenderUnitBuilder &p_builder)
	{
		if (_camera == nullptr || _visibleChunks.empty())
		{
			_pruneUnloadedChunks();
			return;
		}

		const spk::Vector3 cameraPosition = _camera->position();
		std::ranges::sort(_visibleChunks, [&](const auto *p_left, const auto *p_right) {
			const auto squaredDistance = [&](const auto *p_renderer) {
				const spk::Vector3 center = _cache.at(p_renderer).model * (spk::Vector3(p_renderer->grid().size()) * 0.5f);
				return (center - cameraPosition).squaredLength();
			};
			return squaredDistance(p_left) > squaredDistance(p_right);
		});

		for (const spk::VoxelChunkRenderer *renderer : _visibleChunks)
		{
			p_builder.add(std::make_unique<spk::DrawVoxelMesh3DRenderCommand>(renderer->sharedTransparentMesh(), _cache.at(renderer).modelUBO, _sampler, true));
		}
		_pruneUnloadedChunks();
	}
}

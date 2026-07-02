#pragma once

#include "components/camera3d.hpp"
#include "components/transform3d.hpp"
#include "rendering/light_update_render_command.hpp"
#include "rendering/mesh_render_command.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "world/chunk.hpp"

#include <GL/glew.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pg
{
	// Renders chunk meshes with no per-frame GPU allocation. The persistent draw data lives
	// in the logic: the voxel + mask texture samplers are shared by every chunk, and each
	// chunk's two model UBOs (opaque tint + translucent-mask tint) are cached, keyed by the
	// chunk, and rebuilt only when the chunk re-synchronizes (its meshRevision changes).
	// Every frame we assemble lightweight MeshRenderCommands that just reference the cached
	// UBOs/samplers and the chunk's shared mesh, plus a single camera + light update for the
	// whole chunk pass. Sharing the mesh's ownership also keeps the render thread safe if a
	// chunk is re-baked on the update thread mid-frame.
	class ChunkRenderLogic : public spk::ComponentLogic<Chunk>
	{
	private:
		static constexpr GLuint CameraBinding = 1;
		static constexpr float AmbientLight = 0.35f;

		inline static std::atomic<std::size_t> _lastMeshCount{0};
		inline static std::atomic<std::size_t> _lastTriangleCount{0};

		struct CachedDraw
		{
			std::uint64_t meshRevision = 0;
			bool built = false;
			std::shared_ptr<spk::UniformBufferObject> opaqueModelUBO;
			std::shared_ptr<spk::UniformBufferObject> maskModelUBO;
		};

		const spk::Texture &_voxelTexture;
		const spk::Texture *_maskTexture = nullptr;
		std::shared_ptr<spk::SamplerObject> _voxelSampler;
		std::shared_ptr<spk::SamplerObject> _maskSampler;

		Camera3D *_camera = nullptr;
		spk::Matrix4x4 _cameraMatrix;

		std::unordered_map<const Chunk *, CachedDraw> _cache;
		std::vector<const Chunk *> _visibleChunks;
		std::size_t _triangleCount = 0;

		void _rebuild(CachedDraw &p_cached, const Chunk &p_chunk, const spk::Matrix4x4 &p_model)
		{
			p_cached.meshRevision = p_chunk.meshRevision();
			p_cached.built = true;
			p_cached.opaqueModelUBO = MeshRenderCommand::makeModelUBO(p_model, spk::Color{1, 1, 1, 1});
			p_cached.maskModelUBO = MeshRenderCommand::makeModelUBO(p_model, spk::Color{1, 1, 1, 0.65f});
		}

		void _pruneUnloadedChunks()
		{
			if (_cache.size() == _visibleChunks.size())
			{
				return;
			}
			const std::unordered_set<const Chunk *> visible(_visibleChunks.begin(), _visibleChunks.end());
			for (auto iterator = _cache.begin(); iterator != _cache.end();)
			{
				iterator = visible.contains(iterator->first) ? std::next(iterator) : _cache.erase(iterator);
			}
		}

	public:
		explicit ChunkRenderLogic(const spk::Texture &p_voxelTexture, const spk::Texture *p_maskTexture = nullptr) :
			_voxelTexture(p_voxelTexture),
			_maskTexture(p_maskTexture)
		{
		}

		[[nodiscard]] static std::size_t lastMeshCount() noexcept
		{
			return _lastMeshCount.load(std::memory_order_relaxed);
		}

		[[nodiscard]] static std::size_t lastTriangleCount() noexcept
		{
			return _lastTriangleCount.load(std::memory_order_relaxed);
		}

	protected:
		void _onRenderStarted(std::size_t p_componentCount) override
		{
			_visibleChunks.clear();
			_visibleChunks.reserve(p_componentCount);
			_triangleCount = 0;
			_camera = Camera3D::mainCamera();
			if (_camera != nullptr)
			{
				_cameraMatrix = _camera->viewProjectionMatrix();
			}
			if (_voxelSampler == nullptr)
			{
				_voxelSampler = MeshRenderCommand::makeSampler(_voxelTexture);
			}
			if (_maskSampler == nullptr && _maskTexture != nullptr)
			{
				_maskSampler = MeshRenderCommand::makeSampler(*_maskTexture);
			}
		}

		void _parseComponentForRender(Chunk &p_chunk) override
		{
			if (_camera == nullptr || p_chunk.needsSynchronization())
			{
				return;
			}

			spk::Matrix4x4 model = spk::Matrix4x4::identity();
			if (const spk::Entity *owner = p_chunk.entity(); owner != nullptr)
			{
				if (const Transform3D *transform = owner->component<Transform3D>(); transform != nullptr)
				{
					model = transform->modelTransform();
				}
			}

			CachedDraw &cached = _cache[&p_chunk];
			if (!cached.built || cached.meshRevision != p_chunk.meshRevision())
			{
				_rebuild(cached, p_chunk, model);
			}
			_visibleChunks.push_back(&p_chunk);

			if (!p_chunk.renderMesh().indexes().empty())
			{
				_triangleCount += p_chunk.renderMesh().indexes().size() / 3;
			}
			if (_maskTexture != nullptr && !p_chunk.maskMesh().indexes().empty())
			{
				_triangleCount += p_chunk.maskMesh().indexes().size() / 3;
			}
		}

		void _executeRender(spk::RenderUnitBuilder &p_builder) override
		{
			std::size_t meshCount = 0;
			for (const Chunk *chunk : _visibleChunks)
			{
				if (!chunk->renderMesh().indexes().empty())
				{
					++meshCount;
				}
				if (_maskTexture != nullptr && !chunk->maskMesh().indexes().empty())
				{
					++meshCount;
				}
			}
			_lastMeshCount.store(meshCount, std::memory_order_relaxed);
			_lastTriangleCount.store(_triangleCount, std::memory_order_relaxed);

			if (_camera == nullptr || meshCount == 0)
			{
				_pruneUnloadedChunks();
				return;
			}

			p_builder.emplace<spk::CameraUpdateRenderCommand>(CameraBinding, _cameraMatrix);
			p_builder.emplace<LightUpdateRenderCommand>(
				spk::Vector3(1.0f, -2.0f, 0.5f).normalized(),
				spk::Color(1.0f, 0.95f, 0.85f),
				AmbientLight);

			for (const Chunk *chunk : _visibleChunks)
			{
				if (chunk->renderMesh().indexes().empty())
				{
					continue;
				}
				p_builder.add(std::make_unique<MeshRenderCommand>(
					chunk->sharedRenderMesh(), _cache[chunk].opaqueModelUBO, _voxelSampler, false));
			}
			if (_maskTexture != nullptr && _maskSampler != nullptr)
			{
				for (const Chunk *chunk : _visibleChunks)
				{
					if (chunk->maskMesh().indexes().empty())
					{
						continue;
					}
					p_builder.add(std::make_unique<MeshRenderCommand>(
						chunk->sharedMaskMesh(), _cache[chunk].maskModelUBO, _maskSampler, true));
				}
			}

			_pruneUnloadedChunks();
		}
	};
}

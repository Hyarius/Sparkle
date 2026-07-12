#pragma once

#include <GL/glew.h>

#include <atomic>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/game_engine/spk_transform_3d.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"

namespace spk
{
	class TextureMeshRenderLogic : public spk::ComponentLogic<spk::TextureMeshRenderer3D>
	{
	private:
		inline static std::atomic<std::size_t> _lastMeshCount{0};
		inline static std::atomic<std::size_t> _lastTriangleCount{0};

		struct CachedDraw
		{
			const spk::Texture *texture = nullptr;
			std::shared_ptr<spk::SamplerObject> sampler;
			bool hasModelData = false;
			spk::DrawTextureMesh3DRenderCommand::ModelData modelData{};
			std::shared_ptr<spk::UniformBufferObject> modelUBO;
		};

		spk::Camera3D *_camera = nullptr;
		std::optional<spk::RenderPhase> _activePhase;
		std::unordered_map<const spk::TextureMeshRenderer3D *, CachedDraw> _cache;
		std::vector<const spk::TextureMeshRenderer3D *> _seenRenderers;
		std::vector<std::unique_ptr<spk::DrawTextureMesh3DRenderCommand>> _opaqueCommands;
		std::vector<std::unique_ptr<spk::DrawTextureMesh3DRenderCommand>> _translucentCommands;
		std::size_t _triangleCount = 0;
		std::size_t _frameMeshCount = 0;
		std::size_t _frameTriangleCount = 0;

		[[nodiscard]] CachedDraw &_syncCache(
			spk::TextureMeshRenderer3D &p_renderer,
			const spk::Matrix4x4 &p_model)
		{
			CachedDraw &cached = _cache[&p_renderer];
			if (cached.sampler == nullptr || cached.texture != p_renderer.texture())
			{
				cached.texture = p_renderer.texture();
				cached.sampler = spk::DrawTextureMesh3DRenderCommand::makeSampler(*cached.texture);
			}

			const spk::DrawTextureMesh3DRenderCommand::ModelData modelData{
				.model = p_model,
				.tint = p_renderer.tint()};
			if (cached.modelUBO == nullptr)
			{
				cached.modelUBO = spk::DrawTextureMesh3DRenderCommand::makeModelUBO(p_model, p_renderer.tint());
				cached.modelData = modelData;
				cached.hasModelData = true;
			}
			else if (!cached.hasModelData || std::memcmp(&cached.modelData, &modelData, sizeof(modelData)) != 0)
			{
				spk::DrawTextureMesh3DRenderCommand::updateModelUBO(*cached.modelUBO, p_model, p_renderer.tint());
				cached.modelData = modelData;
				cached.hasModelData = true;
			}
			return cached;
		}

		void _pruneUnusedRenderers()
		{
			if (_cache.size() == _seenRenderers.size())
			{
				return;
			}
			const std::unordered_set<const spk::TextureMeshRenderer3D *> seen(
				_seenRenderers.begin(), _seenRenderers.end());
			for (auto iterator = _cache.begin(); iterator != _cache.end();)
			{
				iterator = seen.contains(iterator->first) ? std::next(iterator) : _cache.erase(iterator);
			}
		}

	public:
		TextureMeshRenderLogic() = default;

		[[nodiscard]] spk::RenderPhaseMask renderPhases() const noexcept override
		{
			return spk::renderPhaseBit(spk::RenderPhase::SceneOpaque) |
				   spk::renderPhaseBit(spk::RenderPhase::SceneTransparent);
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
			_activePhase.reset();
			_opaqueCommands.clear();
			_translucentCommands.clear();
			_opaqueCommands.reserve(p_componentCount);
			_translucentCommands.reserve(p_componentCount);
			_seenRenderers.clear();
			_seenRenderers.reserve(p_componentCount);
			_triangleCount = 0;
			_camera = spk::Camera3D::mainCamera();
			_frameMeshCount = 0;
			_frameTriangleCount = 0;
		}

		void _onRenderPhaseStarted(const spk::RenderPhaseContext &p_context, std::size_t p_componentCount) override
		{
			_activePhase = p_context.phase;
			_opaqueCommands.clear();
			_translucentCommands.clear();
			_opaqueCommands.reserve(p_componentCount);
			_translucentCommands.reserve(p_componentCount);
			_seenRenderers.clear();
			_seenRenderers.reserve(p_componentCount);
			_triangleCount = 0;
			if (p_context.phase == spk::RenderPhase::SceneOpaque)
			{
				_frameMeshCount = 0;
				_frameTriangleCount = 0;
			}
			_camera = p_context.frame.mainCamera;
		}

		void _parseComponentForRender(spk::TextureMeshRenderer3D &p_renderer) override
		{
			if (_camera == nullptr || p_renderer.mesh() == nullptr || p_renderer.texture() == nullptr)
			{
				return;
			}

			spk::Matrix4x4 model = spk::Matrix4x4::identity();
			if (spk::Entity *owner = p_renderer.entity(); owner != nullptr)
			{
				if (const spk::Transform3D *transform = owner->component<spk::Transform3D>(); transform != nullptr)
				{
					model = transform->modelTransform();
				}
			}
			const CachedDraw &cached = _syncCache(p_renderer, model);
			_seenRenderers.push_back(&p_renderer);
			const spk::RenderPhase rendererPhase = p_renderer.translucent() ? spk::RenderPhase::SceneTransparent : spk::RenderPhase::SceneOpaque;
			if (_activePhase.has_value() && *_activePhase != rendererPhase)
			{
				return;
			}
			_triangleCount += p_renderer.mesh()->layoutBuffer().indexCount() / 3;
			auto command = std::make_unique<spk::DrawTextureMesh3DRenderCommand>(
				p_renderer.mesh(), cached.modelUBO, cached.sampler, p_renderer.translucent());
			(p_renderer.translucent() ? _translucentCommands : _opaqueCommands).push_back(std::move(command));
		}

		void _executeRender(spk::RenderUnitBuilder &p_builder) override
		{
			const std::size_t meshCount = _opaqueCommands.size() + _translucentCommands.size();
			if (_activePhase.has_value())
			{
				_frameMeshCount += meshCount;
				_frameTriangleCount += _triangleCount;
				_lastMeshCount.store(_frameMeshCount, std::memory_order_relaxed);
				_lastTriangleCount.store(_frameTriangleCount, std::memory_order_relaxed);
			}
			else
			{
				_lastMeshCount.store(meshCount, std::memory_order_relaxed);
				_lastTriangleCount.store(_triangleCount, std::memory_order_relaxed);
			}
			_pruneUnusedRenderers();
			if (meshCount == 0)
			{
				return;
			}

			for (auto &command : _opaqueCommands)
			{
				p_builder.add(std::move(command));
			}
			for (auto &command : _translucentCommands)
			{
				p_builder.add(std::move(command));
			}
			_opaqueCommands.clear();
			_translucentCommands.clear();
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

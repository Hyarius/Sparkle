#pragma once

#include <GL/glew.h>

#include <atomic>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/game_engine/spk_transform_3d.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"

namespace spk
{
	class TextureMeshRenderLogic : public spk::ComponentLogic<spk::TextureMeshRenderer3D>
	{
	private:
		static constexpr GLuint CameraBinding = 1;
		static constexpr GLuint DirectionalLightBinding = 3;
		static constexpr float AmbientLight = 0.35f;

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
		spk::Matrix4x4 _cameraMatrix;
		std::unordered_map<const spk::TextureMeshRenderer3D *, CachedDraw> _cache;
		std::vector<const spk::TextureMeshRenderer3D *> _seenRenderers;
		std::vector<std::unique_ptr<spk::DrawTextureMesh3DRenderCommand>> _opaqueCommands;
		std::vector<std::unique_ptr<spk::DrawTextureMesh3DRenderCommand>> _translucentCommands;
		std::size_t _triangleCount = 0;
		bool _emitFrameState = true;

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
		explicit TextureMeshRenderLogic(bool p_emitFrameState = true) :
			_emitFrameState(p_emitFrameState)
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
			_opaqueCommands.clear();
			_translucentCommands.clear();
			_opaqueCommands.reserve(p_componentCount);
			_translucentCommands.reserve(p_componentCount);
			_seenRenderers.clear();
			_seenRenderers.reserve(p_componentCount);
			_triangleCount = 0;
			_camera = spk::Camera3D::mainCamera();
			if (_camera != nullptr)
			{
				_cameraMatrix = _camera->viewProjectionMatrix();
			}
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
			_triangleCount += p_renderer.mesh()->layoutBuffer().indexCount() / 3;
			const CachedDraw &cached = _syncCache(p_renderer, model);
			_seenRenderers.push_back(&p_renderer);
			auto command = std::make_unique<spk::DrawTextureMesh3DRenderCommand>(
				p_renderer.mesh(), cached.modelUBO, cached.sampler, p_renderer.translucent());
			(p_renderer.translucent() ? _translucentCommands : _opaqueCommands).push_back(std::move(command));
		}

		void _executeRender(spk::RenderUnitBuilder &p_builder) override
		{
			const std::size_t meshCount = _opaqueCommands.size() + _translucentCommands.size();
			_lastMeshCount.store(meshCount, std::memory_order_relaxed);
			_lastTriangleCount.store(_triangleCount, std::memory_order_relaxed);
			_pruneUnusedRenderers();
			if (meshCount == 0)
			{
				return;
			}

			if (_emitFrameState)
			{
				p_builder.emplace<spk::CameraUpdateRenderCommand>(CameraBinding, _cameraMatrix);
				p_builder.emplace<spk::DirectionalLightUpdateRenderCommand>(
					DirectionalLightBinding,
					spk::DirectionalLight{
						.direction = spk::Vector3(1.0f, -2.0f, 0.5f).normalized(),
						.color = spk::Color(1.0f, 0.95f, 0.85f),
						.ambient = AmbientLight});
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
	};
}

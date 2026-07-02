#pragma once

#include "components/camera3d.hpp"
#include "components/transform3d.hpp"
#include "rendering/light_update_render_command.hpp"
#include "rendering/mesh_render_command.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "world/chunk.hpp"

#include <GL/glew.h>

#include <atomic>
#include <memory>
#include <vector>

namespace pg
{
	class ChunkRenderLogic : public spk::ComponentLogic<Chunk>
	{
	private:
		static constexpr GLuint CameraBinding = 1;
		static constexpr float AmbientLight = 0.35f;

		inline static std::atomic<std::size_t> _lastMeshCount{0};
		inline static std::atomic<std::size_t> _lastTriangleCount{0};

		const spk::Texture &_voxelTexture;
		const spk::Texture *_maskTexture = nullptr;
		Camera3D *_camera = nullptr;
		spk::Matrix4x4 _cameraMatrix;
		std::vector<std::unique_ptr<MeshRenderCommand>> _opaqueCommands;
		std::vector<std::unique_ptr<MeshRenderCommand>> _maskCommands;
		std::size_t _triangleCount = 0;

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
			_opaqueCommands.clear();
			_maskCommands.clear();
			_opaqueCommands.reserve(p_componentCount);
			_maskCommands.reserve(p_componentCount);
			_triangleCount = 0;
			_camera = Camera3D::mainCamera();
			if (_camera != nullptr)
			{
				_cameraMatrix = _camera->viewProjectionMatrix();
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

			if (!p_chunk.renderMesh().indexes().empty())
			{
				_triangleCount += p_chunk.renderMesh().indexes().size() / 3;
				_opaqueCommands.push_back(std::make_unique<MeshRenderCommand>(
					p_chunk.renderMesh(), _voxelTexture, model, spk::Color{1, 1, 1, 1}, false));
			}
			if (_maskTexture != nullptr && !p_chunk.maskMesh().indexes().empty())
			{
				_triangleCount += p_chunk.maskMesh().indexes().size() / 3;
				_maskCommands.push_back(std::make_unique<MeshRenderCommand>(
					p_chunk.maskMesh(), *_maskTexture, model, spk::Color{1, 1, 1, 0.65f}, true));
			}
		}

		void _executeRender(spk::RenderUnitBuilder &p_builder) override
		{
			const std::size_t meshCount = _opaqueCommands.size() + _maskCommands.size();
			_lastMeshCount.store(meshCount, std::memory_order_relaxed);
			_lastTriangleCount.store(_triangleCount, std::memory_order_relaxed);
			if (_camera == nullptr || meshCount == 0)
			{
				return;
			}

			p_builder.emplace<spk::CameraUpdateRenderCommand>(CameraBinding, _cameraMatrix);
			p_builder.emplace<LightUpdateRenderCommand>(
				spk::Vector3(1.0f, -2.0f, 0.5f).normalized(),
				spk::Color(1.0f, 0.95f, 0.85f),
				AmbientLight);
			for (auto &command : _opaqueCommands) p_builder.add(std::move(command));
			for (auto &command : _maskCommands) p_builder.add(std::move(command));
			_opaqueCommands.clear();
			_maskCommands.clear();
		}
	};
}

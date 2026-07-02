#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

#include <GL/glew.h>

#include "components/camera3d.hpp"
#include "components/mesh_renderer3d.hpp"
#include "components/transform3d.hpp"
#include "rendering/light_update_render_command.hpp"
#include "rendering/mesh_render_command.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/math/spk_matrix.hpp"

namespace pg
{
	class MeshRenderLogic : public spk::ComponentLogic<MeshRenderer3D>
	{
	private:
		static constexpr GLuint CameraBinding = 1;
		static constexpr float AmbientLight = 0.35f;

		inline static std::atomic<std::size_t> _lastMeshCount{0};
		inline static std::atomic<std::size_t> _lastTriangleCount{0};

		Camera3D *_camera = nullptr;
		spk::Matrix4x4 _cameraMatrix;
		std::vector<std::unique_ptr<MeshRenderCommand>> _opaqueCommands;
		std::vector<std::unique_ptr<MeshRenderCommand>> _translucentCommands;
		std::size_t _triangleCount = 0;
		bool _emitFrameState = true;

	public:
		explicit MeshRenderLogic(bool p_emitFrameState = true) :
			_emitFrameState(p_emitFrameState)
		{
		}

		[[nodiscard]] static std::size_t lastMeshCount()
		{
			return _lastMeshCount.load(std::memory_order_relaxed);
		}

		[[nodiscard]] static std::size_t lastTriangleCount()
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
			_triangleCount = 0;
			_camera = Camera3D::mainCamera();
			if (_camera != nullptr)
			{
				_cameraMatrix = _camera->viewProjectionMatrix();
			}
		}

		void _parseComponentForRender(MeshRenderer3D &p_renderer) override
		{
			if (_camera == nullptr || p_renderer.mesh() == nullptr || p_renderer.texture() == nullptr)
			{
				return;
			}

			spk::Matrix4x4 model = spk::Matrix4x4::identity();
			if (spk::Entity *owner = p_renderer.entity(); owner != nullptr)
			{
				if (const Transform3D *transform = owner->component<Transform3D>(); transform != nullptr)
				{
					model = transform->modelTransform();
				}
			}

			_triangleCount += p_renderer.mesh()->layoutBuffer().indexCount() / 3;
			auto command = std::make_unique<MeshRenderCommand>(
				p_renderer.mesh(),
				MeshRenderCommand::makeModelUBO(model, p_renderer.tint()),
				MeshRenderCommand::makeSampler(*p_renderer.texture()),
				p_renderer.translucent());
			if (p_renderer.translucent())
			{
				_translucentCommands.push_back(std::move(command));
			}
			else
			{
				_opaqueCommands.push_back(std::move(command));
			}
		}

		void _executeRender(spk::RenderUnitBuilder &p_builder) override
		{
			const std::size_t meshCount = _opaqueCommands.size() + _translucentCommands.size();
			_lastMeshCount.store(meshCount, std::memory_order_relaxed);
			_lastTriangleCount.store(_triangleCount, std::memory_order_relaxed);

			if (meshCount == 0)
			{
				return;
			}

			if (_emitFrameState)
			{
				p_builder.emplace<spk::CameraUpdateRenderCommand>(CameraBinding, _cameraMatrix);
				p_builder.emplace<LightUpdateRenderCommand>(
					spk::Vector3(1.0f, -2.0f, 0.5f).normalized(),
					spk::Color(1.0f, 0.95f, 0.85f),
					AmbientLight);
			}

			for (std::unique_ptr<MeshRenderCommand> &command : _opaqueCommands)
			{
				p_builder.add(std::move(command));
			}
			for (std::unique_ptr<MeshRenderCommand> &command : _translucentCommands)
			{
				p_builder.add(std::move(command));
			}
			_opaqueCommands.clear();
			_translucentCommands.clear();
		}
	};
}

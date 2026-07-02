#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

#include <GL/glew.h>

#include "components/camera3d.hpp"
#include "components/mesh_renderer3d.hpp"
#include "components/transform3d.hpp"
#include "rendering/mesh_render_command.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/math/spk_matrix.hpp"

namespace pg
{
	// Render-phase logic for MeshRenderer3D. Mirror of spk::SpriteRenderLogic: selects the
	// camera (main camera for now), uploads its view-projection to UBO binding 1 via a
	// spk::CameraUpdateRenderCommand, then emits one MeshRenderCommand per renderer. Keeping
	// the camera upload in the render process (rather than caching it globally) is what will
	// let us render the same meshes through several cameras later — a per-camera pass emits
	// its own CameraUpdateRenderCommand + viewport before its meshes.
	class MeshRenderLogic : public spk::ComponentLogic<MeshRenderer3D>
	{
	private:
		static constexpr GLuint CameraBinding = 1;

		inline static std::atomic<std::size_t> _lastMeshCount{0};
		inline static std::atomic<std::size_t> _lastTriangleCount{0};

		Camera3D *_camera = nullptr;
		spk::Matrix4x4 _cameraMatrix;
		std::vector<std::unique_ptr<MeshRenderCommand>> _commands;
		std::size_t _triangleCount = 0;

	public:
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
			(void)p_componentCount;
			_commands.clear();
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
			_commands.push_back(std::make_unique<MeshRenderCommand>(*p_renderer.mesh(), *p_renderer.texture(), model));
		}

		void _executeRender(spk::RenderUnitBuilder &p_builder) override
		{
			_lastMeshCount.store(_commands.size(), std::memory_order_relaxed);
			_lastTriangleCount.store(_triangleCount, std::memory_order_relaxed);

			if (_commands.empty())
			{
				return;
			}

			p_builder.emplace<spk::CameraUpdateRenderCommand>(CameraBinding, _cameraMatrix);
			for (std::unique_ptr<MeshRenderCommand> &command : _commands)
			{
				p_builder.add(std::move(command));
			}
			_commands.clear();
		}
	};
}

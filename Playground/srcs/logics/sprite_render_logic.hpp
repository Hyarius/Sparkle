#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include "components/camera2d.hpp"
#include "components/sprite_renderer2d.hpp"
#include "components/transform2d.hpp"
#include "entity2d.hpp"
#include "rendering/camera_update_render_command.hpp"
#include "rendering/instanced_sprite_render_command.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/math/spk_matrix.hpp"

namespace pg
{
	class SpriteRenderLogic : public spk::ComponentLogic<SpriteRenderer2D>
	{
	private:
		static constexpr GLuint CameraBinding = 1;

		inline static std::atomic<std::size_t> _lastSpriteCount{0};
		inline static std::atomic<std::size_t> _lastPolygonCount{0};

		Camera2D *_camera = nullptr;
		spk::Matrix4x4 _cameraMatrix;
		std::vector<std::unique_ptr<InstancedSpriteRenderCommand>> _renderCommands;

		[[nodiscard]] InstancedSpriteRenderCommand &_commandFor(
			const spk::SpriteSheet &p_spriteSheet,
			const spk::TextureMesh2D &p_mesh)
		{
			for (const std::unique_ptr<InstancedSpriteRenderCommand> &command : _renderCommands)
			{
				if (&command->spriteSheet() == &p_spriteSheet && command->mesh().hashKey() == p_mesh.hashKey())
				{
					return *command;
				}
			}

			auto command = std::make_unique<InstancedSpriteRenderCommand>(p_spriteSheet, p_mesh);
			InstancedSpriteRenderCommand &result = *command;
			_renderCommands.push_back(std::move(command));
			return result;
		}

	public:
		[[nodiscard]] static std::size_t lastSpriteCount()
		{
			return _lastSpriteCount.load(std::memory_order_relaxed);
		}

		[[nodiscard]] static std::size_t lastPolygonCount()
		{
			return _lastPolygonCount.load(std::memory_order_relaxed);
		}

	protected:
		void _onRenderStarted(std::size_t p_componentCount) override
		{
			_renderCommands.clear();
			_camera = Camera2D::mainCamera();
			if (_camera != nullptr)
			{
				_cameraMatrix = _camera->projectionMatrix() * _camera->viewMatrix();
			}
		}

		void _parseComponentForRender(SpriteRenderer2D &p_sprite) override
		{
			if (_camera == nullptr ||
				p_sprite.mesh() == nullptr ||
				p_sprite.spriteSheet() == nullptr)
			{
				return;
			}

			if (_camera->viewport().size.x == 0 || _camera->viewport().size.y == 0)
			{
				return;
			}

			const Transform2D &transform = p_sprite.entity()->transform();

			InstancedSpriteRenderCommand &command = _commandFor(*p_sprite.spriteSheet(), *p_sprite.mesh());
			InstancedSpriteRenderCommand::Instance instance = command.pushBackInstance();
			instance.setModelTransform(transform.modelTransform());
			instance.setSprite(p_sprite.sprite());
		}

		void _executeRender(spk::RenderUnitBuilder &p_builder) override
		{
			std::size_t spriteCount = 0;
			for (const std::unique_ptr<InstancedSpriteRenderCommand> &command : _renderCommands)
			{
				spriteCount += command->instanceCount();
			}
			_lastSpriteCount.store(spriteCount, std::memory_order_relaxed);
			_lastPolygonCount.store(spriteCount * 2, std::memory_order_relaxed);

			if (spriteCount == 0)
			{
				_renderCommands.clear();
				return;
			}

			p_builder.emplace<CameraUpdateRenderCommand>(CameraBinding, _cameraMatrix);
			for (std::unique_ptr<InstancedSpriteRenderCommand> &command : _renderCommands)
			{
				p_builder.add(std::move(command));
			}
			_renderCommands.clear();
		}
	};
}

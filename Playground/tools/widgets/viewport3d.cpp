#include "tools/widgets/viewport3d.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "structures/game_engine/spk_texture_mesh_render_logic.hpp"
#include "type/spk_constants.hpp"

namespace pg::tools
{
	Viewport3D::Viewport3D(const std::string &p_name, spk::Widget *p_parent) :
		spk::GameEngineWidget(p_name, p_parent)
	{
		gameEngine().add<spk::TextureMeshRenderLogic>();
		_camera = &_cameraEntity.addComponent<spk::Camera3D>();
		_camera->setPerspective(55.0f, 0.05f, 500.0f);
		_camera->setUp({0.0f, 1.0f, 0.0f});
		_camera->makeMain();
		_cameraEntity.activate();
		gameEngine().addEntity(&_cameraEntity);
		_updateCamera();
	}

	Viewport3D::~Viewport3D()
	{
		clearScene();
		gameEngine().removeEntity(&_cameraEntity);
	}

	void Viewport3D::_updateCamera()
	{
		const float yaw = spk::degreeToRadian(_yaw);
		const float pitch = spk::degreeToRadian(_pitch);
		const float horizontal = std::cos(pitch) * _distance;
		_camera->setTarget(_target);
		_camera->setPosition(_target + spk::Vector3{
			std::sin(yaw) * horizontal,
			std::sin(pitch) * _distance,
			std::cos(yaw) * horizontal});
		invalidateRenderUnit();
	}

	void Viewport3D::_onGeometryChange()
	{
		spk::GameEngineWidget::_onGeometryChange();
		_camera->setViewportSize(
			static_cast<float>(std::max(1u, geometry().width())),
			static_cast<float>(std::max(1u, geometry().height())));
	}

	void Viewport3D::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		spk::GameEngineWidget::_onMouseMovedEvent(p_event);
		if (p_event.device()[spk::Mouse::Right] != spk::InputState::Down)
		{
			return;
		}
		_yaw += static_cast<float>(p_event->delta.x) * 0.35f;
		_pitch = std::clamp(_pitch - static_cast<float>(p_event->delta.y) * 0.3f, -80.0f, 80.0f);
		_updateCamera();
	}

	void Viewport3D::_onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event)
	{
		spk::GameEngineWidget::_onMouseWheelScrolledEvent(p_event);
		_distance = std::clamp(_distance - p_event->delta.y * 0.5f, 1.0f, 100.0f);
		_updateCamera();
	}

	void Viewport3D::setSceneBuilder(SceneBuilder p_builder)
	{
		_sceneBuilder = std::move(p_builder);
		rebuildScene();
	}

	void Viewport3D::rebuildScene()
	{
		clearScene();
		if (_sceneBuilder)
		{
			_sceneBuilder(*this);
		}
		invalidateRenderUnit();
	}

	void Viewport3D::clearScene()
	{
		for (const auto &entity : _sceneEntities)
		{
			gameEngine().removeEntity(entity.get());
		}
		_sceneEntities.clear();
	}

	spk::Entity3D &Viewport3D::createEntity()
	{
		auto entity = std::make_unique<spk::Entity3D>();
		spk::Entity3D &result = *entity;
		entity->activate();
		gameEngine().addEntity(entity.get());
		_sceneEntities.push_back(std::move(entity));
		return result;
	}

	void Viewport3D::frame(const spk::Vector3 &p_target, float p_distance)
	{
		_target = p_target;
		_distance = std::max(1.0f, p_distance);
		_updateCamera();
	}

	void Viewport3D::pan(const spk::Vector2 &p_pixels)
	{
		const spk::Vector3 forward = (_target - _camera->position()).normalized();
		spk::Vector3 right = forward.cross({0.0f, 1.0f, 0.0f});
		if (right.isZero())
		{
			right = {1.0f, 0.0f, 0.0f};
		}
		else
		{
			right = right.normalized();
		}
		const spk::Vector3 up = right.cross(forward).normalized();
		const float scale = _distance * 0.0025f;
		_target += right * (-p_pixels.x * scale) + up * (p_pixels.y * scale);
		_updateCamera();
	}

	spk::Camera3D &Viewport3D::camera() noexcept
	{
		return *_camera;
	}
}

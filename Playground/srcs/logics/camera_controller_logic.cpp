#include "logics/camera_controller_logic.hpp"

#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_transform_3d.hpp"
#include "structures/math/spk_vector3.hpp"
#include "type/spk_constants.hpp"

#include <algorithm>
#include <cmath>

namespace pg
{
	CameraControllerLogic::CameraControllerLogic(GameContext &p_context, spk::Camera3D &p_camera) :
		_context(p_context),
		_camera(p_camera)
	{
	}

	void CameraControllerLogic::_parseComponentForUpdate(const spk::UpdateTick &p_tick, Actor &p_actor)
	{
		if (!_context.world.explorationActive || !p_actor.player)
		{
			_wasActive = false;
			return;
		}
		// Ease from the current camera position when exploration is activated.
		if (!_wasActive)
		{
			_smoothedTarget = _camera.target();
			_smoothedPosition = _camera.position();
			_initialized = true;
			_wasActive = true;
		}
		const float seconds = static_cast<float>(p_tick.deltaTime.seconds());
		if (p_tick.keyboard != nullptr)
		{
			if ((*p_tick.keyboard)[spk::Keyboard::Q] == spk::InputState::Down)
			{
				_yaw -= 75.0f * seconds;
			}
			if ((*p_tick.keyboard)[spk::Keyboard::D] == spk::InputState::Down)
			{
				_yaw += 75.0f * seconds;
			}
			if ((*p_tick.keyboard)[spk::Keyboard::Z] == spk::InputState::Down)
			{
				_pitch += 55.0f * seconds;
			}
			if ((*p_tick.keyboard)[spk::Keyboard::S] == spk::InputState::Down)
			{
				_pitch -= 55.0f * seconds;
			}
		}
		_pitch = std::clamp(_pitch, 20.0f, 75.0f);

		const spk::Transform3D *transform =
			p_actor.entity() == nullptr ? nullptr : p_actor.entity()->component<spk::Transform3D>();
		if (transform == nullptr)
		{
			return;
		}
		const spk::Vector3 desiredTarget = transform->position() + spk::Vector3{0, 0.85f, 0};
		if (!_initialized)
		{
			_smoothedTarget = desiredTarget;
			_initialized = true;
		}
		const float blend = 1.0f - std::exp(-8.0f * seconds);
		_smoothedTarget = spk::Vector3::lerp(_smoothedTarget, desiredTarget, blend);
		const float yaw = spk::degreeToRadian(_yaw);
		const float pitch = spk::degreeToRadian(_pitch);
		const float horizontal = std::cos(pitch) * _distance;
		const spk::Vector3 offset{
			std::sin(yaw) * horizontal,
			std::sin(pitch) * _distance,
			std::cos(yaw) * horizontal};
		const spk::Vector3 desiredPosition = _smoothedTarget + offset;
		_smoothedPosition = spk::Vector3::lerp(_smoothedPosition, desiredPosition, blend);
		_camera.setTarget(_smoothedTarget);
		_camera.setPosition(_smoothedPosition);
	}

	void CameraControllerLogic::_parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, Actor &p_actor)
	{
		if (!_context.world.explorationActive || !p_actor.player ||
			p_event.device()[spk::Mouse::Right] != spk::InputState::Down)
		{
			return;
		}
		_yaw += static_cast<float>(p_event->delta.x) * 0.35f;
		_pitch -= static_cast<float>(p_event->delta.y) * 0.3f;
		_pitch = std::clamp(_pitch, 20.0f, 75.0f);
	}

	void CameraControllerLogic::_parseComponentForMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event, Actor &p_actor)
	{
		if (!_context.world.explorationActive || !p_actor.player)
		{
			return;
		}
		_distance = std::clamp(_distance - p_event->delta.y * 1.5f, 6.0f, 40.0f);
	}
}

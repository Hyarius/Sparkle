#include "battle/presentation/tactical_camera_controller.hpp"

#include "type/spk_constants.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace pg
{
	spk::Vector3 TacticalCameraController::_desiredPosition() const
	{
		const float yaw = spk::degreeToRadian(_state.yawDegrees);
		const float pitch = spk::degreeToRadian(_state.pitchDegrees);
		const float horizontal = std::cos(pitch) * _state.distance;
		return _state.target + spk::Vector3{
								   std::sin(yaw) * horizontal,
								   std::sin(pitch) * _state.distance,
								   std::cos(yaw) * horizontal};
	}

	void TacticalCameraController::_clamp() noexcept
	{
		_state.pitchDegrees = std::clamp(_state.pitchDegrees, 25.0f, 75.0f);
		const spk::Vector3 extent = _bounds.extent();
		const float horizontal = std::max(1.0f, std::max(extent.x, extent.z));
		const float minimum = std::max(4.0f, horizontal * 0.55f);
		const float maximum = std::max(minimum + 1.0f, horizontal * 6.0f + std::max(0.0f, extent.y) * 2.0f);
		_state.distance = std::clamp(_state.distance, minimum, maximum);
	}

	void TacticalCameraController::enter(const PresentationAabb &p_boardBounds, const spk::Camera3D &p_camera)
	{
		if (!p_boardBounds.finite())
		{
			throw std::invalid_argument("tactical camera needs finite board bounds");
		}
		_bounds = p_boardBounds;
		_state.target = p_boardBounds.center();
		const spk::Vector3 extent = p_boardBounds.extent();
		const float horizontal = std::max(1.0f, std::max(extent.x, extent.z));
		const float halfFov = spk::degreeToRadian(std::max(1.0f, p_camera.fieldOfView()) * 0.5f);
		_state.distance = std::max(horizontal * 0.65f / std::tan(halfFov), horizontal * 0.9f) + extent.y * 0.75f;
		_clamp();
		_smoothedTarget = p_camera.target();
		_smoothedPosition = p_camera.position();
		_active = true;
	}

	void TacticalCameraController::update(const spk::UpdateContext &p_tick, spk::Camera3D &p_camera)
	{
		if (!_active)
		{
			return;
		}
		_clamp();
		const float seconds = std::max(0.0f, static_cast<float>(p_tick.deltaTime.seconds()));
		const float blend = 1.0f - std::exp(-8.0f * seconds);
		_smoothedTarget = spk::Vector3::lerp(_smoothedTarget, _state.target, blend);
		_smoothedPosition = spk::Vector3::lerp(_smoothedPosition, _desiredPosition(), blend);
		p_camera.setTarget(_smoothedTarget);
		p_camera.setPosition(_smoothedPosition);
	}

	void TacticalCameraController::onRightDrag(const spk::Vector2Int &p_delta)
	{
		if (!_active)
		{
			return;
		}
		_state.yawDegrees += static_cast<float>(p_delta.x) * 0.35f;
		_state.pitchDegrees -= static_cast<float>(p_delta.y) * 0.3f;
		_clamp();
	}

	void TacticalCameraController::onWheel(float p_delta)
	{
		if (!_active)
		{
			return;
		}
		_state.distance -= p_delta * 1.5f;
		_clamp();
	}

	void TacticalCameraController::beginExit(const spk::Vector3 &p_explorationTarget) noexcept
	{
		// Exploration seeds its own smoothing from the current camera transform on the next update;
		// retaining this target only prevents a tactical update after the ownership hand-off.
		_state.target = p_explorationTarget;
		_active = false;
	}

	void TacticalCameraController::reset() noexcept
	{
		_active = false;
		_bounds = {};
		_state = {};
		_smoothedTarget = {};
		_smoothedPosition = {};
	}

	bool TacticalCameraController::active() const noexcept
	{
		return _active;
	}

	const TacticalCameraState &TacticalCameraController::state() const noexcept
	{
		return _state;
	}
}

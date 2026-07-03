#include "logics/tactical_camera_logic.hpp"

#include "type/spk_constants.hpp"

#include <algorithm>
#include <cmath>

namespace pg
{
	TacticalCameraLogic::TacticalCameraLogic(spk::Camera3D &p_camera) :
		_camera(p_camera)
	{
	}

	TacticalCameraLogic::Pose TacticalCameraLogic::framePose(
		const spk::Vector3Int &p_worldAnchor,
		const spk::Vector2Int &p_boardSize,
		float p_yawDegrees)
	{
		const spk::Vector3 center{
			static_cast<float>(p_worldAnchor.x) + static_cast<float>(p_boardSize.x) * 0.5f,
			static_cast<float>(p_worldAnchor.y) + 2.0f,
			static_cast<float>(p_worldAnchor.z) + static_cast<float>(p_boardSize.y) * 0.5f};
		const float span = static_cast<float>(std::max(p_boardSize.x, p_boardSize.y));
		const float distance = span * 1.15f + 6.0f;
		const float yaw = spk::degreeToRadian(p_yawDegrees);
		const float pitch = spk::degreeToRadian(Pitch);
		const float horizontal = std::cos(pitch) * distance;
		const spk::Vector3 offset{
			std::sin(yaw) * horizontal,
			std::sin(pitch) * distance,
			std::cos(yaw) * horizontal};
		return Pose{center + offset, center};
	}

	TacticalCameraLogic::Pose TacticalCameraLogic::blendPose(const Pose &p_from, const Pose &p_to, float p_t)
	{
		const float t = std::clamp(p_t, 0.0f, 1.0f);
		const float eased = t * t * (3.0f - 2.0f * t); // smoothstep ease in/out
		return Pose{
			spk::Vector3::lerp(p_from.position, p_to.position, eased),
			spk::Vector3::lerp(p_from.target, p_to.target, eased)};
	}

	void TacticalCameraLogic::begin(
		const Pose &p_from,
		const spk::Vector3Int &p_worldAnchor,
		const spk::Vector2Int &p_boardSize,
		float p_yawDegrees)
	{
		_from = p_from;
		_to = framePose(p_worldAnchor, p_boardSize, p_yawDegrees);
		_elapsed = 0.0f;
		_active = true;
	}

	void TacticalCameraLogic::end() noexcept
	{
		_active = false;
	}

	void TacticalCameraLogic::_parseComponentForUpdate(const spk::UpdateTick &p_tick, Actor &p_actor)
	{
		if (!_active || !p_actor.player)
		{
			return;
		}
		_elapsed = std::min(_elapsed + static_cast<float>(p_tick.deltaTime.seconds()), BlendSeconds);
		const Pose pose = blendPose(_from, _to, _elapsed / BlendSeconds);
		_camera.setPosition(pose.position);
		_camera.setTarget(pose.target);
	}
}

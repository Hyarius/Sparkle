#pragma once

#include "components/actor.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

namespace pg
{
	// Battle camera controller. On battle start it eases the shared Camera3D from the exploration
	// pose to a tactical framing of the board (anchor + size -> distance, ~50 deg pitch, yaw from
	// the player's deployment side) over ~0.6 s, then holds it. The return blend is handled by the
	// exploration controller easing back in (camera_controller_logic), so neither cuts (D03).
	class TacticalCameraLogic : public spk::ComponentLogic<Actor>
	{
	public:
		struct Pose
		{
			spk::Vector3 position{};
			spk::Vector3 target{};
		};

		static constexpr float BlendSeconds = 0.6f;
		static constexpr float Pitch = 50.0f;

		// The tactical pose that frames a board. boardSize is the (x, z) extent in cells.
		[[nodiscard]] static Pose framePose(
			const spk::Vector3Int &p_worldAnchor,
			const spk::Vector2Int &p_boardSize,
			float p_yawDegrees);
		// Eased interpolation (smoothstep) between two poses; t clamped to [0, 1].
		[[nodiscard]] static Pose blendPose(const Pose &p_from, const Pose &p_to, float p_t);

	private:
		spk::Camera3D &_camera;
		Pose _from;
		Pose _to;
		float _elapsed = 0.0f;
		bool _active = false;

	public:
		explicit TacticalCameraLogic(spk::Camera3D &p_camera);

		// Start easing from p_from (typically the live exploration pose) to the board framing.
		void begin(
			const Pose &p_from,
			const spk::Vector3Int &p_worldAnchor,
			const spk::Vector2Int &p_boardSize,
			float p_yawDegrees);
		void end() noexcept;

	protected:
		void _parseComponentForUpdate(const spk::UpdateTick &p_tick, Actor &p_actor) override;
	};
}

#pragma once

#include "battle/presentation/battle_presentation_cell_source.hpp"

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/system/event/spk_update_context.hpp"

namespace pg
{
	struct TacticalCameraState
	{
		spk::Vector3 target{};
		float yawDegrees = 35.0f;
		float pitchDegrees = 48.0f;
		float distance = 16.0f;
	};

	class TacticalCameraController
	{
	private:
		TacticalCameraState _state;
		PresentationAabb _bounds;
		spk::Vector3 _smoothedTarget{};
		spk::Vector3 _smoothedPosition{};
		bool _active = false;

		[[nodiscard]] spk::Vector3 _desiredPosition() const;
		void _clamp() noexcept;

	public:
		void enter(const PresentationAabb &p_boardBounds, const spk::Camera3D &p_camera);
		void update(const spk::UpdateContext &p_tick, spk::Camera3D &p_camera);
		void onRightDrag(const spk::Vector2Int &p_delta);
		void onWheel(float p_delta);
		void beginExit(const spk::Vector3 &p_explorationTarget) noexcept;
		void reset() noexcept;

		[[nodiscard]] bool active() const noexcept;
		[[nodiscard]] const TacticalCameraState &state() const noexcept;
	};
}

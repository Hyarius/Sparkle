#pragma once

#include "components/actor.hpp"
#include "core/game_context.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"

namespace pg
{
	class CameraControllerLogic : public spk::ComponentLogic<Actor>
	{
	public:
		[[nodiscard]] spk::RenderPhaseMask renderPhases() const noexcept override
		{
			return spk::RenderPhaseMask::None;
		}

	private:
		GameContext &_context;
		spk::Camera3D &_camera;
		float _yaw = 225.0f;
		float _pitch = 42.0f;
		float _distance = 18.0f;
		spk::Vector3 _smoothedTarget{};
		spk::Vector3 _smoothedPosition{};
		bool _initialized = false;
		bool _wasActive = false;

	public:
		CameraControllerLogic(GameContext &p_context, spk::Camera3D &p_camera);

		// Shifts the whole rig (smoothed target, position and the camera itself) by the
		// given world delta in one step, keeping yaw/pitch/distance: used when the actor
		// teleports so the camera does not swoop across the world to catch up.
		void teleportBy(const spk::Vector3 &p_delta);

	protected:
		void _parseComponentForUpdate(const spk::UpdateContext &p_tick, Actor &p_actor) override;
		void _parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, Actor &p_actor) override;
		void _parseComponentForMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event, Actor &p_actor) override;
	};
}

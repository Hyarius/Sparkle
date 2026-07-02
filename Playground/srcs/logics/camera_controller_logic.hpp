#pragma once

#include "components/actor.hpp"
#include "components/camera3d.hpp"
#include "core/game_context.hpp"
#include "structures/game_engine/spk_component_logic.hpp"

namespace pg
{
	class CameraControllerLogic : public spk::ComponentLogic<Actor>
	{
	private:
		GameContext &_context;
		Camera3D &_camera;
		float _yaw = 225.0f;
		float _pitch = 42.0f;
		float _distance = 18.0f;
		spk::Vector3 _smoothedTarget{};
		bool _initialized = false;

	public:
		CameraControllerLogic(GameContext &p_context, Camera3D &p_camera);

	protected:
		void _parseComponentForUpdate(const spk::UpdateTick &p_tick, Actor &p_actor) override;
		void _parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, Actor &p_actor) override;
		void _parseComponentForMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event, Actor &p_actor) override;
	};
}

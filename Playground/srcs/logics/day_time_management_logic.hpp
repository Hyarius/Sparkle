#pragma once

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_light_3d.hpp"

namespace spk { class SceneLightingRenderFeature; }

namespace pg
{
	// Drives the Playground sun through 24 artistic hours every 60 real seconds,
	// with an asymmetric cycle: daylight 08:00 -> 21:00, night the remaining 11 h.
	class DayTimeManagementLogic final : public spk::ComponentLogic<spk::Light3D>
	{
	private:
		spk::Entity3D &_sunEntity;
		spk::SceneLightingRenderFeature &_lighting;
		float _timeOfDayHours = 11.0f;

	public:
		DayTimeManagementLogic(spk::Entity3D &p_sunEntity, spk::SceneLightingRenderFeature &p_lighting);
		[[nodiscard]] float timeOfDayHours() const noexcept;

	protected:
		void _parseComponentForUpdate(const spk::UpdateContext &p_tick, spk::Light3D &p_light) override;
	};
}

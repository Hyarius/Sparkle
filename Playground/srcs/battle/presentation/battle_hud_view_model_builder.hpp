#pragma once

#include "battle/battle_snapshot.hpp"
#include "battle/presentation/battle_hud_view_model.hpp"
#include "battle/presentation/battle_interaction_controller.hpp"

namespace pg
{
	class Registries;

	class BattleHudViewModelBuilder
	{
	public:
		// The builder copies all display data and has no reference/pointer member.  Interaction is
		// only read to mirror accepted selection state; it never submits or mutates a command.
		[[nodiscard]] static BattleHudViewModel build(
			const BattleSnapshot &p_snapshot,
			const Registries &p_registries,
			const BattleInteractionState &p_interaction);
		[[nodiscard]] static BattleHudViewModel build(
			const BattleSnapshot &p_snapshot,
			const Registries &p_registries,
			const BattleInteractionController &p_interaction);
	};
}

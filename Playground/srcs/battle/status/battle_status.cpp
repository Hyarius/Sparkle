#include "battle/status/battle_status.hpp"

namespace pg
{
	BattleStatusSnapshot snapshotStatus(const BattleStatusInstance &p_status)
	{
		return {
			.id = p_status.id,
			.definitionId = p_status.definitionId,
			.origin = p_status.origin,
			.stacks = p_status.stacks,
			.duration = snapshotDuration(p_status.duration),
			.appliedBy = p_status.appliedBy,
			.sourceAbilityId = p_status.sourceAbilityId,
			.sourceEffectId = p_status.sourceEffectId};
	}
}

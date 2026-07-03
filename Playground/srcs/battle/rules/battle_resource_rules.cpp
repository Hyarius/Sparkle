#include "battle/rules/battle_resource_rules.hpp"
#include "battle/battle_unit.hpp"
#include "battle/rules/battle_status_rules.hpp"
#include "statuses/status.hpp"

namespace pg
{
	BattleResourceChangeResult BattleResourceRules::change(BattleUnit &p_unit, BattleResourceKind p_resource, int p_delta)
	{
		ObservableResource *resource = nullptr;
		switch (p_resource)
		{
		case BattleResourceKind::Health:
			resource = &p_unit.attributes.hp;
			break;
		case BattleResourceKind::AP:
			resource = &p_unit.attributes.ap;
			break;
		case BattleResourceKind::MP:
			resource = &p_unit.attributes.mp;
			break;
		default:
			return {};
		}
		const int before = resource->current();
		resource->change(p_delta);
		const int actual = resource->current() - before;
		const BattleResourceChangeResult result{
			.gained = std::max(0, actual), .lost = std::max(0, -actual)};
		if (result.lost > 0)
		{
			const StatusHookPoint hook = p_resource == BattleResourceKind::Health ? StatusHookPoint::OnHPLoss
																				  : (p_resource == BattleResourceKind::AP ? StatusHookPoint::OnAPLoss : StatusHookPoint::OnMPLoss);
			BattleStatusRules::applyHook(p_unit, hook, result.lost);
		}
		else if (result.gained > 0)
		{
			const StatusHookPoint hook = p_resource == BattleResourceKind::Health ? StatusHookPoint::OnHPGain
																				  : (p_resource == BattleResourceKind::AP ? StatusHookPoint::OnAPGain : StatusHookPoint::OnMPGain);
			BattleStatusRules::applyHook(p_unit, hook, result.gained);
		}
		return result;
	}
}

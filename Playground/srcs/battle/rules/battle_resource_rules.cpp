#include "battle/rules/battle_resource_rules.hpp"
#include "battle/battle_unit.hpp"

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
		return {.gained = std::max(0, actual), .lost = std::max(0, -actual)};
	}
}

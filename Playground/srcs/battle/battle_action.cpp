#include "battle/battle_action.hpp"
#include "abilities/ability.hpp"
namespace pg
{
	int AbilityAction::apCost() const noexcept
	{
		return ability.apCost;
	}
	int AbilityAction::mpCost() const noexcept
	{
		return ability.mpCost;
	}
}

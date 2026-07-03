#include "abilities/effect.hpp"

#include "abilities/effects/effect_types.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{
	std::string number(float p_value)
	{
		std::ostringstream stream;
		stream << std::fixed << std::setprecision(2) << p_value;
		std::string result = stream.str();
		while (result.size() > 1 && result.back() == '0')
		{
			result.pop_back();
		}
		if (!result.empty() && result.back() == '.')
		{
			result.pop_back();
		}
		return result;
	}

	std::string duration(const pg::Duration &p_duration)
	{
		switch (p_duration.kind)
		{
		case pg::DurationKind::TurnBased:
			return std::to_string(p_duration.turns) + (p_duration.turns == 1 ? " turn" : " turns");
		case pg::DurationKind::Seconds:
			return number(p_duration.seconds) + " seconds";
		case pg::DurationKind::Infinite:
			return "indefinitely";
		}
		return {};
	}

	std::string resource(pg::EffectResource p_resource)
	{
		switch (p_resource)
		{
		case pg::EffectResource::Health:
			return "health";
		case pg::EffectResource::AP:
			return "AP";
		case pg::EffectResource::MP:
			return "MP";
		case pg::EffectResource::Range:
			return "range";
		case pg::EffectResource::TurnBarTime:
			return "turn-bar time";
		}
		return {};
	}

	std::string tags(const std::vector<std::string> &p_tags)
	{
		std::string result;
		for (const auto &tag : p_tags)
		{
			if (!result.empty())
			{
				result += ", ";
			}
			result += tag;
		}
		return result;
	}
}

namespace pg
{
	std::string describe(const Effect &p_effect)
	{
		if (const auto *e = dynamic_cast<const DamageEffect *>(&p_effect))
		{
			return "Deals " + std::to_string(e->baseDamage) + " " + (e->damageKind == DamageKind::Physical ? "physical" : "magical") +
				   " damage (" + number(e->attackRatio) + " attack, " + number(e->magicRatio) + " magic).";
		}
		if (const auto *e = dynamic_cast<const HealEffect *>(&p_effect))
		{
			return "Restores " + std::to_string(e->baseHealing) + " health (" + number(e->attackRatio) + " attack, " + number(e->magicRatio) + " magic).";
		}
		if (const auto *e = dynamic_cast<const ApplyStatusEffect *>(&p_effect))
		{
			return "Applies " + std::to_string(e->stackCount) + " stack(s) of " + e->status + " for " + duration(e->duration) + ".";
		}
		if (const auto *e = dynamic_cast<const RemoveStatusEffect *>(&p_effect))
		{
			return "Removes " + std::to_string(e->stackCount) + " stack(s) of " + e->status + ".";
		}
		if (const auto *e = dynamic_cast<const ConsumeStatusEffect *>(&p_effect))
		{
			return "Consumes " + std::to_string(e->stackCount) + " stack(s) of " + e->status + ".";
		}
		if (const auto *e = dynamic_cast<const CleanseEffect *>(&p_effect))
		{
			return "Cleanses statuses tagged " + tags(e->tags) + ".";
		}
		if (const auto *e = dynamic_cast<const ReviveEffect *>(&p_effect))
		{
			return "Revives the target with " + std::to_string(e->healthRestored) + " health.";
		}
		if (const auto *e = dynamic_cast<const ApplyShieldEffect *>(&p_effect))
		{
			return "Applies a " + std::to_string(e->amount) + "-point " + (e->kind == ShieldKind::Physical ? "physical" : "magical") +
				   " shield for " + (e->durationTurns < 0 ? std::string("an unlimited duration") : std::to_string(e->durationTurns) + " turn(s)") + ".";
		}
		if (const auto *e = dynamic_cast<const ResourceChangeEffect *>(&p_effect))
		{
			return "Changes the target's " + resource(e->resource) + " by " + std::to_string(e->value) + ".";
		}
		if (const auto *e = dynamic_cast<const StealResourceEffect *>(&p_effect))
		{
			return "Steals up to " + std::to_string(e->value) + " " + resource(e->resource) + ".";
		}
		if (const auto *e = dynamic_cast<const DisplacementEffect *>(&p_effect))
		{
			return std::string(e->orientation == DisplacementOrientation::TowardCaster ? "Pulls" : "Pushes") + " the target up to " + std::to_string(e->force) + " cell(s).";
		}
		if (dynamic_cast<const SwapPositionEffect *>(&p_effect))
		{
			return "Swaps the target with the unit at the anchor cell.";
		}
		if (dynamic_cast<const SwapPositionWithCasterEffect *>(&p_effect))
		{
			return "Swaps the target's position with the caster.";
		}
		if (dynamic_cast<const TeleportSelfEffect *>(&p_effect))
		{
			return "Teleports the caster to the anchor cell.";
		}
		if (const auto *e = dynamic_cast<const AdjustTurnBarTimeEffect *>(&p_effect))
		{
			return "Adjusts turn-bar time by " + number(e->delta) + " seconds.";
		}
		if (const auto *e = dynamic_cast<const AdjustTurnBarDurationEffect *>(&p_effect))
		{
			return "Adjusts turn-bar duration by " + number(e->delta) + " seconds.";
		}
		if (const auto *e = dynamic_cast<const PlaceObjectEffect *>(&p_effect))
		{
			return "Places " + e->object + " for " + duration(e->duration) + ".";
		}
		if (const auto *e = dynamic_cast<const RemoveObjectEffect *>(&p_effect))
		{
			return "Removes objects tagged " + tags(e->tags) + ".";
		}
		throw std::invalid_argument("unknown effect type");
	}
}

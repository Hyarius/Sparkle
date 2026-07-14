#include "feats/feat_reward_definition.hpp"

#include "core/content_id.hpp"
#include "core/definition_fields.hpp"

namespace pg
{
	namespace
	{
		enum class FeatRewardKind
		{
			BonusStat,
			UnlockAbility,
			RemoveAbility,
			UnlockPassive,
			ChangeForm
		};

		[[nodiscard]] const std::map<std::string, FeatRewardKind> &featRewardKindNames()
		{
			static const std::map<std::string, FeatRewardKind> names{
				{"bonusStat", FeatRewardKind::BonusStat},
				{"changeForm", FeatRewardKind::ChangeForm},
				{"removeAbility", FeatRewardKind::RemoveAbility},
				{"unlockAbility", FeatRewardKind::UnlockAbility},
				{"unlockPassive", FeatRewardKind::UnlockPassive}};
			return names;
		}

		[[nodiscard]] std::string requireReferencedId(
			const JsonReader &p_reader,
			const std::string &p_key,
			std::string_view p_kind)
		{
			std::string value = p_reader.require<std::string>(p_key);
			requireContentId(value, p_reader.file(), p_reader.pathFor(p_key), p_kind);
			return value;
		}

		[[nodiscard]] BonusStatRewardSpec parseBonusStat(JsonReader &p_reader)
		{
			BonusStatRewardSpec spec;
			spec.stat = p_reader.requireEnum<CreatureStat>("stat", creatureStatNames());

			if (spec.stat == CreatureStat::Stamina)
			{
				p_reader.forbidUnknown({"id", "type", "stat", "amountSeconds"});

				spec.staminaDelta = requireBattleSeconds(p_reader, "amountSeconds", BattleTimeSign::Any);
				if (spec.staminaDelta.isZero())
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("amountSeconds"),
						"a stamina reward of zero grants nothing");
				}
				return spec;
			}

			p_reader.forbidUnknown({"id", "type", "stat", "amount"});

			spec.amount =
				requireIntegerInRange(p_reader, "amount", -MaximumBonusStatAmount, MaximumBonusStatAmount);
			if (spec.amount == 0)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("amount"), "a stat reward of zero grants nothing");
			}
			return spec;
		}

		[[nodiscard]] FeatRewardPayload parsePayload(JsonReader &p_reader, FeatRewardKind p_kind)
		{
			switch (p_kind)
			{
			case FeatRewardKind::BonusStat:
			{
				// Stamina is milliseconds behind a seconds key, so its allowed keys differ; the
				// bonusStat parser calls forbidUnknown itself once it knows the stat.
				return parseBonusStat(p_reader);
			}
			case FeatRewardKind::UnlockAbility:
			{
				p_reader.forbidUnknown({"id", "type", "ability"});

				UnlockAbilityRewardSpec spec;
				spec.ability = requireReferencedId(p_reader, "ability", "ability id");
				return spec;
			}
			case FeatRewardKind::RemoveAbility:
			{
				p_reader.forbidUnknown({"id", "type", "ability"});

				RemoveAbilityRewardSpec spec;
				spec.ability = requireReferencedId(p_reader, "ability", "ability id");
				return spec;
			}
			case FeatRewardKind::UnlockPassive:
			{
				p_reader.forbidUnknown({"id", "type", "status"});

				UnlockPassiveRewardSpec spec;
				spec.status = requireReferencedId(p_reader, "status", "status id");
				return spec;
			}
			case FeatRewardKind::ChangeForm:
			{
				p_reader.forbidUnknown({"id", "type", "form"});

				ChangeFormRewardSpec spec;
				// Syntax now, resolution in step 04: which forms exist is a property of the
				// species that selects this board, not of the board.
				spec.form = requireReferencedId(p_reader, "form", "form id");
				return spec;
			}
			}

			throw JsonError(p_reader.file(), p_reader.path(), "unhandled reward type");
		}
	}

	FeatRewardSpec parseFeatRewardSpec(JsonReader &p_reader)
	{
		const FeatRewardKind kind = p_reader.requireEnum<FeatRewardKind>("type", featRewardKindNames());

		FeatRewardSpec result;
		result.id = p_reader.require<std::string>("id");
		requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "reward id");
		result.source = DefinitionSource{p_reader.file(), p_reader.path()};
		result.payload = parsePayload(p_reader, kind);
		return result;
	}

	std::vector<FeatRewardSpec> parseFeatRewards(const JsonReader &p_reader, const std::string &p_key)
	{
		std::vector<JsonReader> entries = p_reader.childArray(p_key);

		std::vector<FeatRewardSpec> result;
		result.reserve(entries.size());
		for (JsonReader &entry : entries)
		{
			result.push_back(parseFeatRewardSpec(entry));
		}
		return result;
	}
}

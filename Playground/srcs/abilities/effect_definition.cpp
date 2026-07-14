#include "abilities/effect_definition.hpp"

#include "core/content_id.hpp"
#include "core/definition_fields.hpp"

#include <string_view>

namespace pg
{
	namespace
	{
		enum class EffectKind
		{
			Damage,
			Heal,
			ApplyShield,
			ApplyStatus,
			RemoveStatus,
			Cleanse,
			ChangeResource,
			ApplyNextActivationPenalty,
			AdjustTurnBar,
			Displace,
			SwapWithSource,
			TeleportSource,
			PlaceObject,
			RemoveObjects
		};

		[[nodiscard]] const std::map<std::string, EffectKind> &effectKindNames()
		{
			static const std::map<std::string, EffectKind> names{
				{"adjustTurnBar", EffectKind::AdjustTurnBar},
				{"applyNextActivationPenalty", EffectKind::ApplyNextActivationPenalty},
				{"applyShield", EffectKind::ApplyShield},
				{"applyStatus", EffectKind::ApplyStatus},
				{"changeResource", EffectKind::ChangeResource},
				{"cleanse", EffectKind::Cleanse},
				{"damage", EffectKind::Damage},
				{"displace", EffectKind::Displace},
				{"heal", EffectKind::Heal},
				{"placeObject", EffectKind::PlaceObject},
				{"removeObjects", EffectKind::RemoveObjects},
				{"removeStatus", EffectKind::RemoveStatus},
				{"swapWithSource", EffectKind::SwapWithSource},
				{"teleportSource", EffectKind::TeleportSource}};
			return names;
		}

		[[nodiscard]] bool isUnitScope(EffectApplyTo p_applyTo) noexcept
		{
			return p_applyTo == EffectApplyTo::SourceUnit || p_applyTo == EffectApplyTo::PrimaryUnit ||
				   p_applyTo == EffectApplyTo::AffectedUnits;
		}

		[[nodiscard]] bool isCellScope(EffectApplyTo p_applyTo) noexcept
		{
			return p_applyTo == EffectApplyTo::AnchorCell || p_applyTo == EffectApplyTo::AffectedCells;
		}

		// An invalid scope is never silently reinterpreted: a unit effect authored on a cell,
		// or the reverse, is an authoring bug that must surface at load.
		void requireScope(const JsonReader &p_reader, EffectKind p_kind, EffectApplyTo p_applyTo)
		{
			const auto reject = [&p_reader](std::string_view p_expectation) {
				throw JsonError(p_reader.file(), p_reader.pathFor("applyTo"), std::string(p_expectation));
			};

			switch (p_kind)
			{
			case EffectKind::SwapWithSource:
				if (p_applyTo != EffectApplyTo::PrimaryUnit)
				{
					reject("swapWithSource trades cells with exactly one unit, so it applies to 'primaryUnit' only");
				}
				return;
			case EffectKind::TeleportSource:
				if (p_applyTo != EffectApplyTo::AnchorCell)
				{
					reject("teleportSource moves the source to the chosen anchor, so it applies to 'anchorCell' only");
				}
				return;
			case EffectKind::PlaceObject:
			case EffectKind::RemoveObjects:
				if (!isCellScope(p_applyTo))
				{
					reject("this effect acts on cells, so it applies to 'anchorCell' or 'affectedCells'");
				}
				return;
			default:
				if (!isUnitScope(p_applyTo))
				{
					reject("this effect acts on units, so it applies to 'sourceUnit', 'primaryUnit' or 'affectedUnits'");
				}
				return;
			}
		}

		[[nodiscard]] std::int64_t requireMagnitude(
			const JsonReader &p_reader,
			const std::string &p_key,
			std::int64_t p_minimum)
		{
			return requireIntegerInRange(p_reader, p_key, p_minimum, MaximumEffectMagnitude);
		}

		[[nodiscard]] std::int32_t requireRatioPermille(const JsonReader &p_reader, const std::string &p_key)
		{
			return static_cast<std::int32_t>(
				requireIntegerInRange(p_reader, p_key, 0, MaximumEffectRatioPermille));
		}

		// A damage or heal effect that is entirely zero would be a no-op the runtime still has
		// to schedule, publish and animate. Reject it at authoring time.
		void requireSomeMagnitude(
			const JsonReader &p_reader,
			std::int64_t p_base,
			std::int32_t p_strengthRatio,
			std::int32_t p_magicPowerRatio)
		{
			if (p_base <= 0 && p_strengthRatio <= 0 && p_magicPowerRatio <= 0)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.path(),
					"at least one of base, strengthRatioPermille or magicPowerRatioPermille must be positive");
			}
		}

		[[nodiscard]] DurationSpec parseDuration(const JsonReader &p_reader)
		{
			JsonReader durationReader = p_reader.child("duration");
			return parseDurationSpec(durationReader);
		}

		[[nodiscard]] EffectPayload parsePayload(JsonReader &p_reader, EffectKind p_kind)
		{
			switch (p_kind)
			{
			case EffectKind::Damage:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "applyTo",
					 "requiresLivingSource",
					 "kind",
					 "base",
					 "strengthRatioPermille",
					 "magicPowerRatioPermille"});

				DamageEffectSpec spec;
				spec.kind = p_reader.requireEnum<DamageKind>("kind", damageKindNames());
				spec.base = requireMagnitude(p_reader, "base", 0);
				spec.strengthRatioPermille = requireRatioPermille(p_reader, "strengthRatioPermille");
				spec.magicPowerRatioPermille = requireRatioPermille(p_reader, "magicPowerRatioPermille");
				requireSomeMagnitude(p_reader, spec.base, spec.strengthRatioPermille, spec.magicPowerRatioPermille);
				return spec;
			}
			case EffectKind::Heal:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "applyTo",
					 "requiresLivingSource",
					 "base",
					 "strengthRatioPermille",
					 "magicPowerRatioPermille"});

				HealEffectSpec spec;
				spec.base = requireMagnitude(p_reader, "base", 0);
				spec.strengthRatioPermille = requireRatioPermille(p_reader, "strengthRatioPermille");
				spec.magicPowerRatioPermille = requireRatioPermille(p_reader, "magicPowerRatioPermille");
				requireSomeMagnitude(p_reader, spec.base, spec.strengthRatioPermille, spec.magicPowerRatioPermille);
				return spec;
			}
			case EffectKind::ApplyShield:
			{
				p_reader.forbidUnknown(
					{"id", "type", "applyTo", "requiresLivingSource", "kind", "amount", "duration"});

				ApplyShieldEffectSpec spec;
				spec.kind = p_reader.requireEnum<DamageKind>("kind", damageKindNames());
				spec.amount = requireMagnitude(p_reader, "amount", 1);
				spec.duration = parseDuration(p_reader);
				return spec;
			}
			case EffectKind::ApplyStatus:
			{
				p_reader.forbidUnknown(
					{"id", "type", "applyTo", "requiresLivingSource", "status", "stacks", "duration"});

				ApplyStatusEffectSpec spec;
				spec.status = p_reader.require<std::string>("status");
				requireContentId(spec.status, p_reader.file(), p_reader.pathFor("status"), "status id");
				spec.stacks = static_cast<std::uint32_t>(requireMagnitude(p_reader, "stacks", 1));
				spec.duration = parseDuration(p_reader);
				return spec;
			}
			case EffectKind::RemoveStatus:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource", "status", "stacks"});

				RemoveStatusEffectSpec spec;
				spec.status = p_reader.require<std::string>("status");
				requireContentId(spec.status, p_reader.file(), p_reader.pathFor("status"), "status id");
				spec.stacks = static_cast<std::uint32_t>(requireMagnitude(p_reader, "stacks", 1));
				return spec;
			}
			case EffectKind::Cleanse:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource", "tags"});

				CleanseEffectSpec spec;
				spec.tags = requireTags(p_reader, "tags", false);
				return spec;
			}
			case EffectKind::ChangeResource:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource", "resource", "delta"});

				ChangeResourceEffectSpec spec;
				spec.resource = p_reader.requireEnum<BattleResource>("resource", battleResourceNames());
				spec.delta =
					requireIntegerInRange(p_reader, "delta", -MaximumEffectMagnitude, MaximumEffectMagnitude);
				if (spec.delta == 0)
				{
					throw JsonError(p_reader.file(), p_reader.pathFor("delta"), "a resource change of zero does nothing");
				}
				return spec;
			}
			case EffectKind::ApplyNextActivationPenalty:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource", "resource", "amount"});

				ApplyNextActivationPenaltyEffectSpec spec;
				spec.resource = p_reader.requireEnum<BattleResource>("resource", battleResourceNames());
				// Only ever a penalty: a bonus refill is a status with a stat modifier.
				spec.amount = requireMagnitude(p_reader, "amount", 1);
				return spec;
			}
			case EffectKind::AdjustTurnBar:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource", "deltaSeconds"});

				AdjustTurnBarEffectSpec spec;
				spec.delta = requireBattleSeconds(p_reader, "deltaSeconds", BattleTimeSign::Any);
				if (spec.delta.isZero())
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("deltaSeconds"),
						"a turn-bar adjustment of zero does nothing");
				}
				return spec;
			}
			case EffectKind::Displace:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource", "direction", "distance"});

				DisplaceEffectSpec spec;
				spec.direction = p_reader.requireEnum<DisplaceDirection>("direction", displaceDirectionNames());
				spec.distance =
					static_cast<int>(requireIntegerInRange(p_reader, "distance", 1, MaximumDisplaceDistance));
				return spec;
			}
			case EffectKind::SwapWithSource:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource"});
				return SwapWithSourceEffectSpec{};
			}
			case EffectKind::TeleportSource:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource"});
				return TeleportSourceEffectSpec{};
			}
			case EffectKind::PlaceObject:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource", "object", "duration"});

				PlaceObjectEffectSpec spec;
				spec.object = p_reader.require<std::string>("object");
				requireContentId(spec.object, p_reader.file(), p_reader.pathFor("object"), "battle object id");
				spec.duration = parseDuration(p_reader);
				return spec;
			}
			case EffectKind::RemoveObjects:
			{
				p_reader.forbidUnknown({"id", "type", "applyTo", "requiresLivingSource", "tags"});

				RemoveObjectsEffectSpec spec;
				spec.tags = requireTags(p_reader, "tags", false);
				return spec;
			}
			}

			throw JsonError(p_reader.file(), p_reader.path(), "unhandled effect type");
		}
	}

	const std::map<std::string, EffectApplyTo> &effectApplyToNames()
	{
		static const std::map<std::string, EffectApplyTo> names{
			{"affectedCells", EffectApplyTo::AffectedCells},
			{"affectedUnits", EffectApplyTo::AffectedUnits},
			{"anchorCell", EffectApplyTo::AnchorCell},
			{"primaryUnit", EffectApplyTo::PrimaryUnit},
			{"sourceUnit", EffectApplyTo::SourceUnit}};
		return names;
	}

	const std::map<std::string, DisplaceDirection> &displaceDirectionNames()
	{
		static const std::map<std::string, DisplaceDirection> names{
			{"awayFromSource", DisplaceDirection::AwayFromSource},
			{"towardSource", DisplaceDirection::TowardSource}};
		return names;
	}

	EffectApplication parseEffectApplication(JsonReader &p_reader)
	{
		// The type is read first: it selects the alternative, and only the selected parser
		// knows which fields the object may carry.
		const EffectKind kind = p_reader.requireEnum<EffectKind>("type", effectKindNames());

		EffectApplication result;
		result.id = p_reader.require<std::string>("id");
		requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "effect id");
		result.applyTo = p_reader.requireEnum<EffectApplyTo>("applyTo", effectApplyToNames());
		result.requiresLivingSource = p_reader.require<bool>("requiresLivingSource");
		result.source = EffectSource{p_reader.file(), p_reader.path()};

		requireScope(p_reader, kind, result.applyTo);
		result.payload = parsePayload(p_reader, kind);
		return result;
	}

	std::vector<EffectApplication> parseEffects(const JsonReader &p_reader, const std::string &p_key)
	{
		std::vector<JsonReader> entries = p_reader.childArray(p_key);
		if (entries.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "expected at least one effect");
		}

		std::vector<EffectApplication> result;
		result.reserve(entries.size());
		for (JsonReader &entry : entries)
		{
			result.push_back(parseEffectApplication(entry));
		}
		return result;
	}

	void requireUniqueEffectIds(const std::vector<EffectApplication> &p_effects, std::set<std::string> &p_seenIds)
	{
		for (const EffectApplication &effect : p_effects)
		{
			if (!p_seenIds.insert(effect.id).second)
			{
				throw JsonError(
					effect.source.file,
					effect.source.jsonPath,
					"duplicate effect id '" + effect.id + "' in this definition");
			}
		}
	}
}

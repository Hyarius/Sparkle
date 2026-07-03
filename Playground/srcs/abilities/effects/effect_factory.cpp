#include "abilities/effect.hpp"

#include "abilities/effects/effect_types.hpp"
#include "core/json.hpp"
#include "core/registry.hpp"

#include <map>

namespace
{
	using namespace pg;

	template <typename TEffect>
	std::unique_ptr<TEffect> make()
	{
		return std::make_unique<TEffect>();
	}

	void requireNonEmpty(const JsonReader &p_reader, const std::string &p_key, const std::string &p_value)
	{
		if (p_value.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), p_key + " must not be empty");
		}
	}

	std::vector<std::string> parseTags(JsonReader &p_reader)
	{
		auto tags = p_reader.require<std::vector<std::string>>("tags");
		if (tags.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("tags"), "tags must not be empty");
		}
		for (const std::string &tag : tags)
		{
			requireNonEmpty(p_reader, "tags", tag);
		}
		return tags;
	}

	EffectResource parseResource(JsonReader &p_reader, bool p_allowHealthAndTime)
	{
		static const std::map<std::string, EffectResource> common = {
			{"ap", EffectResource::AP}, {"mp", EffectResource::MP}, {"range", EffectResource::Range}};
		static const std::map<std::string, EffectResource> steal = {
			{"health", EffectResource::Health}, {"ap", EffectResource::AP}, {"mp", EffectResource::MP}, {"range", EffectResource::Range}, {"turnBarTime", EffectResource::TurnBarTime}};
		return p_allowHealthAndTime ? p_reader.requireEnum<EffectResource>("resource", steal)
									: p_reader.requireEnum<EffectResource>("resource", common);
	}
}

namespace pg
{
	Duration parseDuration(JsonReader &p_reader)
	{
		const std::string type = p_reader.require<std::string>("type");
		if (type == "turnBased")
		{
			p_reader.forbidUnknown({"type", "turns"});
			const int turns = p_reader.require<int>("turns");
			if (turns <= 0)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("turns"), "turn count must be positive");
			}
			return {.kind = DurationKind::TurnBased, .turns = turns};
		}
		if (type == "seconds")
		{
			p_reader.forbidUnknown({"type", "seconds"});
			const float seconds = p_reader.require<float>("seconds");
			if (seconds <= 0.0f)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("seconds"), "seconds must be positive");
			}
			return {.kind = DurationKind::Seconds, .seconds = seconds};
		}
		if (type == "infinite")
		{
			p_reader.forbidUnknown({"type"});
			return {};
		}
		throw JsonError(p_reader.file(), p_reader.pathFor("type"), "unknown duration type '" + type + "'");
	}

	PolymorphicFactory<Effect> &effectFactory()
	{
		static PolymorphicFactory<Effect> factory;
		static const bool registered = [] {
			factory.registerType("damage", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "baseDamage", "damageKind", "attackRatio", "magicRatio"});
				static const std::map<std::string, DamageKind> kinds = {{"physical", DamageKind::Physical}, {"magical", DamageKind::Magical}};
				auto e = make<DamageEffect>();
				e->baseDamage = r.require<int>("baseDamage");
				e->damageKind = r.requireEnum<DamageKind>("damageKind", kinds);
				e->attackRatio = r.require<float>("attackRatio");
				e->magicRatio = r.require<float>("magicRatio");
				if (e->baseDamage < 0 || e->attackRatio < 0 || e->magicRatio < 0)
				{
					throw JsonError(r.file(), r.path(), "damage values must be non-negative");
				}
				return e;
			});
			factory.registerType("heal", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "baseHealing", "attackRatio", "magicRatio"});
				auto e = make<HealEffect>();
				e->baseHealing = r.require<int>("baseHealing");
				e->attackRatio = r.require<float>("attackRatio");
				e->magicRatio = r.require<float>("magicRatio");
				if (e->baseHealing < 0 || e->attackRatio < 0 || e->magicRatio < 0)
				{
					throw JsonError(r.file(), r.path(), "healing values must be non-negative");
				}
				return e;
			});
			factory.registerType("applyStatus", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "status", "stackCount", "duration"});
				auto e = make<ApplyStatusEffect>();
				e->status = r.require<std::string>("status");
				requireNonEmpty(r, "status", e->status);
				e->stackCount = r.optional<int>("stackCount", 1);
				JsonReader d = r.child("duration");
				e->duration = parseDuration(d);
				if (e->stackCount <= 0)
				{
					throw JsonError(r.file(), r.pathFor("stackCount"), "stack count must be positive");
				}
				return e;
			});
			factory.registerType("removeStatus", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "status", "stackCount"});
				auto e = make<RemoveStatusEffect>();
				e->status = r.require<std::string>("status");
				requireNonEmpty(r, "status", e->status);
				e->stackCount = r.optional<int>("stackCount", 1);
				if (e->stackCount <= 0)
				{
					throw JsonError(r.file(), r.pathFor("stackCount"), "stack count must be positive");
				}
				return e;
			});
			factory.registerType("consumeStatus", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "status", "stackCount"});
				auto e = make<ConsumeStatusEffect>();
				e->status = r.require<std::string>("status");
				requireNonEmpty(r, "status", e->status);
				e->stackCount = r.optional<int>("stackCount", 1);
				if (e->stackCount <= 0)
				{
					throw JsonError(r.file(), r.pathFor("stackCount"), "stack count must be positive");
				}
				return e;
			});
			factory.registerType("cleanse", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "tags"});
				auto e = make<CleanseEffect>();
				e->tags = parseTags(r);
				return e;
			});
			factory.registerType("revive", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "healthRestored"});
				auto e = make<ReviveEffect>();
				e->healthRestored = r.optional<int>("healthRestored", 1);
				if (e->healthRestored <= 0)
				{
					throw JsonError(r.file(), r.pathFor("healthRestored"), "health restored must be positive");
				}
				return e;
			});
			factory.registerType("applyShield", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "kind", "amount", "durationTurns"});
				static const std::map<std::string, ShieldKind> kinds = {{"physical", ShieldKind::Physical}, {"magical", ShieldKind::Magical}};
				auto e = make<ApplyShieldEffect>();
				e->kind = r.requireEnum<ShieldKind>("kind", kinds);
				e->amount = r.require<int>("amount");
				e->durationTurns = r.optional<int>("durationTurns", -1);
				if (e->amount <= 0 || (e->durationTurns < 1 && e->durationTurns != -1))
				{
					throw JsonError(r.file(), r.path(), "invalid shield amount or duration");
				}
				return e;
			});
			factory.registerType("resourceChange", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "resource", "value"});
				auto e = make<ResourceChangeEffect>();
				e->resource = parseResource(r, false);
				e->value = r.require<int>("value");
				return e;
			});
			factory.registerType("stealResource", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "resource", "value"});
				auto e = make<StealResourceEffect>();
				e->resource = parseResource(r, true);
				e->value = r.require<int>("value");
				if (e->value <= 0)
				{
					throw JsonError(r.file(), r.pathFor("value"), "steal value must be positive");
				}
				return e;
			});
			factory.registerType("displacement", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "orientation", "force"});
				static const std::map<std::string, DisplacementOrientation> values = {{"towardCaster", DisplacementOrientation::TowardCaster}, {"awayFromCaster", DisplacementOrientation::AwayFromCaster}};
				auto e = make<DisplacementEffect>();
				e->orientation = r.requireEnum<DisplacementOrientation>("orientation", values);
				e->force = r.require<int>("force");
				if (e->force <= 0)
				{
					throw JsonError(r.file(), r.pathFor("force"), "force must be positive");
				}
				return e;
			});
			factory.registerType("swapPosition", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type"});
				return make<SwapPositionEffect>();
			});
			factory.registerType("swapPositionWithCaster", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type"});
				return make<SwapPositionWithCasterEffect>();
			});
			factory.registerType("teleportSelf", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type"});
				return make<TeleportSelfEffect>();
			});
			factory.registerType("adjustTurnBarTime", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "delta"});
				auto e = make<AdjustTurnBarTimeEffect>();
				e->delta = r.require<float>("delta");
				return e;
			});
			factory.registerType("adjustTurnBarDuration", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "delta"});
				auto e = make<AdjustTurnBarDurationEffect>();
				e->delta = r.require<float>("delta");
				return e;
			});
			factory.registerType("placeObject", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "object", "duration"});
				auto e = make<PlaceObjectEffect>();
				e->object = r.require<std::string>("object");
				requireNonEmpty(r, "object", e->object);
				JsonReader d = r.child("duration");
				e->duration = parseDuration(d);
				return e;
			});
			factory.registerType("removeObject", [](JsonReader &r) -> std::unique_ptr<Effect> {
				r.forbidUnknown({"type", "tags"});
				auto e = make<RemoveObjectEffect>();
				e->tags = parseTags(r);
				return e;
			});
			return true;
		}();
		(void)registered;
		return factory;
	}

	std::unique_ptr<Effect> parseEffect(JsonReader &p_reader)
	{
		return effectFactory().parse(p_reader);
	}
}

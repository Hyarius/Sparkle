#pragma once

#include "structures/math/spk_vector3.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace pg
{
	struct Ability;
	struct BattleObject;
	class BattleContext;
	class JsonReader;
	template <typename TBase>
	class PolymorphicFactory;

	enum class DurationKind
	{
		TurnBased,
		Seconds,
		Infinite
	};

	struct Duration
	{
		DurationKind kind = DurationKind::Infinite;
		int turns = 0;
		float seconds = 0.0f;
	};

	struct BattleAbilityExecutionContext
	{
		BattleContext &context;
		const Ability *ability = nullptr;
		BattleObject *sourceObject = nullptr;
		BattleObject *targetObject = nullptr;
		spk::Vector3Int anchorCell{};
		spk::Vector3Int affectedCell{};
		int triggerAmount = 0;
		float mitigationScaling = 10.0f;
		float timeEffectResistanceScaling = 10.0f;
		float minimumTurnBarDuration = 0.1f;
	};

	class Effect
	{
	public:
		virtual ~Effect() = default;
		[[nodiscard]] virtual std::string_view type() const noexcept = 0;
		virtual void apply(BattleAbilityExecutionContext &p_context) const = 0;
	};

	[[nodiscard]] Duration parseDuration(JsonReader &p_reader);
	[[nodiscard]] PolymorphicFactory<Effect> &effectFactory();
	[[nodiscard]] std::unique_ptr<Effect> parseEffect(JsonReader &p_reader);
	[[nodiscard]] std::string describe(const Effect &p_effect);
}

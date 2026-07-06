#pragma once

#include <memory>
#include <string>

namespace pg
{
	class CreatureUnit;
	class JsonReader;
	struct Ability;
	struct Status;
	template <typename TDefinition> class Registry;

	class FeatReward
	{
	public:
		virtual ~FeatReward() = default;
		virtual void apply(CreatureUnit &p_unit) const = 0;
	};

	class BonusStatsReward final : public FeatReward
	{
	public:
		std::string attribute;
		float value = 0.0f;
		void apply(CreatureUnit &p_unit) const override;
	};

	class AbilityReward final : public FeatReward
	{
	public:
		const Ability *ability = nullptr;
		bool remove = false;
		void apply(CreatureUnit &p_unit) const override;
	};

	class PassiveReward final : public FeatReward
	{
	public:
		const Status *status = nullptr;
		void apply(CreatureUnit &p_unit) const override;
	};

	class ChangeFormReward final : public FeatReward
	{
	public:
		std::string form;
		void apply(CreatureUnit &p_unit) const override;
	};

	[[nodiscard]] std::unique_ptr<FeatReward> parseFeatReward(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities,
		const Registry<Status> &p_statuses);
}

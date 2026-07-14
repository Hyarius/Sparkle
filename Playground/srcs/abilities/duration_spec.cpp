#include "abilities/duration_spec.hpp"

#include "core/definition_fields.hpp"

namespace pg
{
	const std::map<std::string, DurationKind> &durationKindNames()
	{
		static const std::map<std::string, DurationKind> names{
			{"infinite", DurationKind::Infinite},
			{"ownerActivations", DurationKind::OwnerActivations},
			{"timeline", DurationKind::Timeline}};
		return names;
	}

	DurationSpec parseDurationSpec(JsonReader &p_reader)
	{
		// The type selects the alternative, and only then does the reader know which keys the
		// object is allowed to carry: "seconds" on an ownerActivations duration is a typo, not
		// an extra.
		const DurationKind kind = p_reader.requireEnum<DurationKind>("type", durationKindNames());
		switch (kind)
		{
		case DurationKind::Timeline:
		{
			p_reader.forbidUnknown({"type", "seconds"});
			return TimelineDurationSpec{requireBattleSeconds(p_reader, "seconds", BattleTimeSign::Positive)};
		}
		case DurationKind::OwnerActivations:
		{
			p_reader.forbidUnknown({"type", "count"});
			const std::int64_t count =
				requireIntegerInRange(p_reader, "count", MinimumOwnerActivations, MaximumOwnerActivations);
			return OwnerActivationsDurationSpec{static_cast<std::uint32_t>(count)};
		}
		case DurationKind::Infinite:
		{
			p_reader.forbidUnknown({"type"});
			return InfiniteDurationSpec{};
		}
		}

		throw JsonError(p_reader.file(), p_reader.path(), "unhandled duration type");
	}
}

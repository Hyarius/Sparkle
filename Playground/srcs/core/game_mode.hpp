#pragma once

#include "structures/container/spk_observable_value.hpp"

namespace pg
{
	// The single authority for input and simulation ownership.  WorldContext deliberately has no
	// second mutable "exploration active" latch: a battle either owns control or exploration does.
	enum class GameModeKind
	{
		Exploration,
		Battle
	};

	struct ControlContext
	{
		spk::ObservableValue<GameModeKind> mode{GameModeKind::Exploration};

		[[nodiscard]] bool isExploration() const noexcept
		{
			return mode.value() == GameModeKind::Exploration;
		}

		[[nodiscard]] bool isBattle() const noexcept
		{
			return mode.value() == GameModeKind::Battle;
		}
	};
}

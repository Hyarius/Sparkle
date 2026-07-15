#include "battle/battle_event.hpp"

#include <variant>

namespace pg
{
	std::string_view toString(BattleEventCategory p_category) noexcept
	{
		switch (p_category)
		{
		case BattleEventCategory::Gameplay:
			return "gameplay";
		case BattleEventCategory::Lifecycle:
			return "lifecycle";
		case BattleEventCategory::Taming:
			return "taming";
		case BattleEventCategory::Diagnostic:
			return "diagnostic";
		}
		return "lifecycle";
	}

	std::string_view battleEventName(const BattleEventPayload &p_payload) noexcept
	{
		struct Namer
		{
			std::string_view operator()(const BattleStarted &) const
			{
				return "battleStarted";
			}
			std::string_view operator()(const UnitDeploymentChanged &) const
			{
				return "unitDeploymentChanged";
			}
			std::string_view operator()(const DeploymentConfirmed &) const
			{
				return "deploymentConfirmed";
			}
			std::string_view operator()(const DeploymentCompleted &) const
			{
				return "deploymentCompleted";
			}
			std::string_view operator()(const BattleTimeAdvanced &) const
			{
				return "battleTimeAdvanced";
			}
			std::string_view operator()(const ActivationStarted &) const
			{
				return "activationStarted";
			}
			std::string_view operator()(const ActivationEnded &) const
			{
				return "activationEnded";
			}
			std::string_view operator()(const ResourceSpent &) const
			{
				return "resourceSpent";
			}
			std::string_view operator()(const ResourceChanged &) const
			{
				return "resourceChanged";
			}
			std::string_view operator()(const NextActivationPenaltyApplied &) const
			{
				return "nextActivationPenaltyApplied";
			}
			std::string_view operator()(const TurnBarAdjusted &) const
			{
				return "turnBarAdjusted";
			}
			std::string_view operator()(const UnitMovementStep &) const
			{
				return "unitMovementStep";
			}
			std::string_view operator()(const UnitMoved &) const
			{
				return "unitMoved";
			}
			std::string_view operator()(const UnitDisplaced &) const
			{
				return "unitDisplaced";
			}
			std::string_view operator()(const UnitTeleported &) const
			{
				return "unitTeleported";
			}
			std::string_view operator()(const UnitsSwapped &) const
			{
				return "unitsSwapped";
			}
			std::string_view operator()(const AbilityCast &) const
			{
				return "abilityCast";
			}
			std::string_view operator()(const Damage &) const
			{
				return "damage";
			}
			std::string_view operator()(const Healing &) const
			{
				return "healing";
			}
			std::string_view operator()(const ShieldApplied &) const
			{
				return "shieldApplied";
			}
			std::string_view operator()(const ShieldAbsorbed &) const
			{
				return "shieldAbsorbed";
			}
			std::string_view operator()(const ShieldBroken &) const
			{
				return "shieldBroken";
			}
			std::string_view operator()(const ShieldRemoved &) const
			{
				return "shieldRemoved";
			}
			std::string_view operator()(const StatusApplied &) const
			{
				return "statusApplied";
			}
			std::string_view operator()(const StatusRemoved &) const
			{
				return "statusRemoved";
			}
			std::string_view operator()(const EffectiveStatChanged &) const
			{
				return "effectiveStatChanged";
			}
			std::string_view operator()(const BattleObjectPlaced &) const
			{
				return "battleObjectPlaced";
			}
			std::string_view operator()(const BattleObjectRemoved &) const
			{
				return "battleObjectRemoved";
			}
			std::string_view operator()(const BattleObjectTriggered &) const
			{
				return "battleObjectTriggered";
			}
			std::string_view operator()(const EffectApplicationSkipped &) const
			{
				return "effectApplicationSkipped";
			}
			std::string_view operator()(const UnitRemoved &) const
			{
				return "unitRemoved";
			}
			std::string_view operator()(const UnitDefeated &) const
			{
				return "unitDefeated";
			}
			std::string_view operator()(const BattleEnded &) const
			{
				return "battleEnded";
			}
			std::string_view operator()(const BattleAborted &) const
			{
				return "battleAborted";
			}
		};
		return std::visit(Namer{}, p_payload);
	}
}

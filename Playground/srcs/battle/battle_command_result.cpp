#include "battle/battle_command_result.hpp"

namespace pg
{
	std::string_view toString(BattleConstructionErrorCode p_code) noexcept
	{
		switch (p_code)
		{
		case BattleConstructionErrorCode::InvalidDescriptor:
			return "invalidDescriptor";
		case BattleConstructionErrorCode::InvalidParticipantCount:
			return "invalidParticipantCount";
		case BattleConstructionErrorCode::DuplicateParticipantIdentity:
			return "duplicateParticipantIdentity";
		case BattleConstructionErrorCode::InvalidRosterOrder:
			return "invalidRosterOrder";
		case BattleConstructionErrorCode::UnknownDefinitionReference:
			return "unknownDefinitionReference";
		case BattleConstructionErrorCode::InvalidDerivedLoadout:
			return "invalidDerivedLoadout";
		case BattleConstructionErrorCode::BoardSourceMismatch:
			return "boardSourceMismatch";
		case BattleConstructionErrorCode::InsufficientDeploymentCapacity:
			return "insufficientDeploymentCapacity";
		case BattleConstructionErrorCode::EnemyPlacementInvalid:
			return "enemyPlacementInvalid";
		case BattleConstructionErrorCode::NumericOverflow:
			return "numericOverflow";
		}
		return "invalidDescriptor";
	}

	std::string_view toString(CommandRejection p_reason) noexcept
	{
		switch (p_reason)
		{
		case CommandRejection::SessionBusy:
			return "sessionBusy";
		case CommandRejection::BattleTerminal:
			return "battleTerminal";
		case CommandRejection::WrongPhase:
			return "wrongPhase";
		case CommandRejection::WrongController:
			return "wrongController";
		case CommandRejection::UnknownUnit:
			return "unknownUnit";
		case CommandRejection::UnitRemoved:
			return "unitRemoved";
		case CommandRejection::UnitAlreadyConfirmed:
			return "unitAlreadyConfirmed";
		case CommandRejection::UnitOutsideDeploymentZone:
			return "unitOutsideDeploymentZone";
		case CommandRejection::DestinationNotStandable:
			return "destinationNotStandable";
		case CommandRejection::DestinationOccupied:
			return "destinationOccupied";
		case CommandRejection::DeploymentIncomplete:
			return "deploymentIncomplete";
		case CommandRejection::CommandUnavailable:
			return "commandUnavailable";
		case CommandRejection::NoStateChange:
			return "noStateChange";
		case CommandRejection::UnitNotPlaced: return "unitNotPlaced";
		case CommandRejection::NotActiveUnit: return "notActiveUnit";
		case CommandRejection::UnknownBoardCell: return "unknownBoardCell";
		case CommandRejection::UnknownAbility: return "unknownAbility";
		case CommandRejection::AbilityNotOwned: return "abilityNotOwned";
		case CommandRejection::InsufficientActionPoints: return "insufficientActionPoints";
		case CommandRejection::InsufficientMovementPoints: return "insufficientMovementPoints";
		case CommandRejection::DestinationBlocked: return "destinationBlocked";
		case CommandRejection::DestinationUnreachable: return "destinationUnreachable";
		case CommandRejection::AnchorOutOfRange: return "anchorOutOfRange";
		case CommandRejection::AnchorLineOfSightBlocked: return "anchorLineOfSightBlocked";
		case CommandRejection::AnchorFilterRejected: return "anchorFilterRejected";
		case CommandRejection::EffectRuntimeUnavailable: return "effectRuntimeUnavailable";
		}
		return "commandUnavailable";
	}
}

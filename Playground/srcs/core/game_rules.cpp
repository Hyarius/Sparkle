#include "core/game_rules.hpp"

#include <array>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <string_view>

namespace pg
{
	namespace
	{
		constexpr int SupportedVersion = 2;

		// The eight overlay categories the battle presentation draws. All are required and no
		// other key is accepted: a typo must not silently leave a category unpainted.
		constexpr std::array<std::string_view, 8> OverlayMaskKeys{
			"deployment",
			"movement",
			"path",
			"abilityRange",
			"areaOfEffect",
			"losBlocked",
			"hovered",
			"invalid"};

		[[nodiscard]] std::int64_t requireIntegerInRange(
			const JsonReader &p_reader,
			const std::string &p_key,
			std::int64_t p_minimum,
			std::int64_t p_maximum)
		{
			const std::int64_t value = p_reader.require<std::int64_t>(p_key);
			if (value < p_minimum || value > p_maximum)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor(p_key),
					"expected an integer in [" + std::to_string(p_minimum) + ", " + std::to_string(p_maximum) +
						"], got " + std::to_string(value));
			}
			return value;
		}

		[[nodiscard]] std::size_t requireExactCount(
			const JsonReader &p_reader,
			const std::string &p_key,
			std::size_t p_expected)
		{
			const std::int64_t value = p_reader.require<std::int64_t>(p_key);
			if (value != static_cast<std::int64_t>(p_expected))
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor(p_key),
					"expected exactly " + std::to_string(p_expected) + ", got " + std::to_string(value) +
						": this is a serialized invariant of the runtime, not a tunable");
			}
			return p_expected;
		}

		[[nodiscard]] int requireBoardSide(const JsonReader &p_reader, const std::string &p_key, int p_side, std::string_view p_axis)
		{
			if (p_side < BattleGameRules::MinimumBoardSide || p_side > BattleGameRules::MaximumBoardSide ||
				p_side % 2 == 0)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor(p_key),
					"board " + std::string(p_axis) + " must be an odd integer in [" +
						std::to_string(BattleGameRules::MinimumBoardSide) + ", " +
						std::to_string(BattleGameRules::MaximumBoardSide) + "], got " + std::to_string(p_side));
			}
			return p_side;
		}

		[[nodiscard]] BattleGameRules parseBattleGameRules(JsonReader &p_reader)
		{
			p_reader.forbidUnknown(
				{"teamCapacity",
				 "abilitySlotCapacity",
				 "defaultBoardSize",
				 "deploymentDepth",
				 "minimumStaminaSeconds",
				 "mitigationScale",
				 "maxCommandsPerActivation",
				 "maxAiCommandsPerActivation",
				 "maxEffectChainDepth",
				 "maxConditionDepth"});

			BattleGameRules result;
			result.teamCapacity = requireExactCount(p_reader, "teamCapacity", BattleGameRules::RequiredTeamCapacity);
			result.abilitySlotCapacity =
				requireExactCount(p_reader, "abilitySlotCapacity", BattleGameRules::RequiredAbilitySlotCapacity);

			const std::array<int, 2> boardSize = p_reader.require<std::array<int, 2>>("defaultBoardSize");
			result.defaultBoardSize[0] = requireBoardSide(p_reader, "defaultBoardSize", boardSize[0], "width");
			result.defaultBoardSize[1] = requireBoardSide(p_reader, "defaultBoardSize", boardSize[1], "depth");

			result.deploymentDepth = static_cast<int>(
				requireIntegerInRange(p_reader, "deploymentDepth", 1, BattleGameRules::MaximumBoardSide));
			// Both sides deploy from their own edge; without a neutral row between the strips
			// the two rosters would start already interleaved.
			if (2 * result.deploymentDepth >= result.defaultBoardSize[1])
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("deploymentDepth"),
					"two deployment strips of " + std::to_string(result.deploymentDepth) +
						" rows leave no neutral row on a board " + std::to_string(result.defaultBoardSize[1]) +
						" deep");
			}

			result.minimumStamina = requireBattleSeconds(p_reader, "minimumStaminaSeconds", BattleTimeSign::Positive);
			result.mitigationScale = requireIntegerInRange(
				p_reader,
				"mitigationScale",
				BattleGameRules::MinimumMitigationScale,
				BattleGameRules::MaximumMitigationScale);

			result.maxCommandsPerActivation = static_cast<std::size_t>(
				requireIntegerInRange(p_reader, "maxCommandsPerActivation", 1, std::numeric_limits<std::int64_t>::max()));
			result.maxAiCommandsPerActivation = static_cast<std::size_t>(requireIntegerInRange(
				p_reader,
				"maxAiCommandsPerActivation",
				1,
				static_cast<std::int64_t>(result.maxCommandsPerActivation)));
			result.maxEffectChainDepth = static_cast<std::size_t>(requireIntegerInRange(
				p_reader,
				"maxEffectChainDepth",
				1,
				static_cast<std::int64_t>(BattleGameRules::MaximumEffectChainDepth)));
			result.maxConditionDepth = static_cast<std::size_t>(requireIntegerInRange(
				p_reader,
				"maxConditionDepth",
				1,
				static_cast<std::int64_t>(BattleGameRules::MaximumConditionDepth)));
			return result;
		}

		[[nodiscard]] std::map<std::string, std::array<int, 2>> parseOverlayMasks(JsonReader &p_reader)
		{
			p_reader.forbidUnknown(
				{"deployment",
				 "movement",
				 "path",
				 "abilityRange",
				 "areaOfEffect",
				 "losBlocked",
				 "hovered",
				 "invalid"});

			std::map<std::string, std::array<int, 2>> result;
			for (const std::string_view key : OverlayMaskKeys)
			{
				const std::string name(key);
				const std::array<int, 2> cell = p_reader.require<std::array<int, 2>>(name);
				if (cell[0] < 0 || cell[1] < 0)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor(name),
						"mask atlas cells are non-negative, got [" + std::to_string(cell[0]) + ", " +
							std::to_string(cell[1]) + "]");
				}
				result.emplace(name, cell);
			}
			return result;
		}
	}

	GameRules parseGameRules(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "maxVerticalTraversalGap", "battle", "overlayMasks"});
		if (p_reader.require<int>("version") != SupportedVersion)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("version"),
				"unsupported version (expected " + std::to_string(SupportedVersion) + ")");
		}

		GameRules result;
		result.maxVerticalTraversalGap = p_reader.require<double>("maxVerticalTraversalGap");
		if (!std::isfinite(result.maxVerticalTraversalGap) || result.maxVerticalTraversalGap < 0.0)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("maxVerticalTraversalGap"),
				"expected a finite, non-negative number of world units");
		}

		JsonReader battleReader = p_reader.child("battle");
		result.battle = parseBattleGameRules(battleReader);

		JsonReader overlayMasksReader = p_reader.child("overlayMasks");
		result.overlayMasks = parseOverlayMasks(overlayMasksReader);
		return result;
	}
}

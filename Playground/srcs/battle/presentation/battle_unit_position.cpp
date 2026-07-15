#include "battle/presentation/battle_unit_position.hpp"

#include "board/board_data.hpp"
#include "board/cell_source.hpp"

#include <cmath>

namespace
{
	[[nodiscard]] std::uint64_t fnv1a(std::string_view p_text) noexcept
	{
		std::uint64_t value = 14695981039346656037ULL;
		for (const unsigned char character : p_text)
		{
			value ^= character;
			value *= 1099511628211ULL;
		}
		return value;
	}
}

namespace pg
{
	spk::Color toSpkColor(const PlaceholderVisual &p_visual) noexcept
	{
		constexpr float channel = 1.0f / 255.0f;
		return {
			static_cast<float>(p_visual.tint[0]) * channel,
			static_cast<float>(p_visual.tint[1]) * channel,
			static_cast<float>(p_visual.tint[2]) * channel,
			static_cast<float>(p_visual.tint[3]) * channel};
	}

	float toWorldScale(const PlaceholderVisual &p_visual) noexcept
	{
		const float scale = static_cast<float>(p_visual.scalePermille) / 1000.0f;
		return std::isfinite(scale) && scale > 0.0f ? scale : 1.0f;
	}

	PlaceholderVisual fallbackPlaceholderVisual(std::string_view p_semanticId) noexcept
	{
		const std::uint64_t hash = fnv1a(p_semanticId);
		// Keep every channel away from black while remaining completely deterministic across
		// standard-library implementations and unrelated battle RNG state.
		return {.tint = {static_cast<std::uint8_t>(72U + (hash & 0x7fU)), static_cast<std::uint8_t>(72U + ((hash >> 8U) & 0x7fU)), static_cast<std::uint8_t>(72U + ((hash >> 16U) & 0x7fU)), 255U}, .scalePermille = 1000};
	}

	spk::Vector3 battleUnitCenterPosition(
		const BoardData &p_board,
		const BoardCell &p_localSupport,
		float p_renderedHeight)
	{
		const float height = std::isfinite(p_renderedHeight) && p_renderedHeight > 0.0f
								 ? p_renderedHeight
								 : BattleUnitBaseEdge;
		spk::Vector3 result = p_board.unitPresentationPosition(p_localSupport);
		result.y += height * 0.5f + BattleUnitFootClearance;
		return result;
	}

	spk::Vector3 battleUnitSegmentCenterPosition(
		const BoardData &p_board,
		const BoardCell &p_from,
		const BoardCell &p_to,
		float p_progress,
		float p_renderedHeight)
	{
		const float height = std::isfinite(p_renderedHeight) && p_renderedHeight > 0.0f
								 ? p_renderedHeight
								 : BattleUnitBaseEdge;
		spk::Vector3 result = interpolateWalkSegment(p_board.cells(), p_from, p_to, p_progress);
		const spk::Vector3Int origin = p_board.presentationOrigin();
		result += spk::Vector3(origin);
		result.y += height * 0.5f + BattleUnitFootClearance;
		return result;
	}
}

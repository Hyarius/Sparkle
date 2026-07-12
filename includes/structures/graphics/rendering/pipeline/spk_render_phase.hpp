#pragma once

#include <cstdint>
#include <string_view>

namespace spk
{
	enum class RenderPhase : std::uint8_t
	{
		PassSetup,
		ShadowCaster,
		SceneOpaque,
		SceneTransparent,
		SceneOverlay,
		Count
	};

	enum class RenderPhaseMask : std::uint32_t
	{
		None = 0
	};

	[[nodiscard]] constexpr RenderPhaseMask operator|(RenderPhaseMask p_lhs, RenderPhaseMask p_rhs) noexcept
	{
		return static_cast<RenderPhaseMask>(
			static_cast<std::uint32_t>(p_lhs) | static_cast<std::uint32_t>(p_rhs));
	}

	[[nodiscard]] constexpr RenderPhaseMask operator&(RenderPhaseMask p_lhs, RenderPhaseMask p_rhs) noexcept
	{
		return static_cast<RenderPhaseMask>(
			static_cast<std::uint32_t>(p_lhs) & static_cast<std::uint32_t>(p_rhs));
	}

	constexpr RenderPhaseMask &operator|=(RenderPhaseMask &p_lhs, RenderPhaseMask p_rhs) noexcept
	{
		p_lhs = p_lhs | p_rhs;
		return p_lhs;
	}

	[[nodiscard]] constexpr RenderPhaseMask operator~(RenderPhaseMask p_mask) noexcept
	{
		return static_cast<RenderPhaseMask>(~static_cast<std::uint32_t>(p_mask));
	}

	[[nodiscard]] constexpr RenderPhaseMask renderPhaseBit(RenderPhase p_phase) noexcept
	{
		return p_phase < RenderPhase::Count ? static_cast<RenderPhaseMask>(std::uint32_t{1} << static_cast<unsigned>(p_phase)) : RenderPhaseMask::None;
	}

	[[nodiscard]] constexpr bool containsRenderPhase(RenderPhaseMask p_mask, RenderPhase p_phase) noexcept
	{
		return (p_mask & renderPhaseBit(p_phase)) != RenderPhaseMask::None;
	}

	[[nodiscard]] constexpr RenderPhaseMask allRenderPhases() noexcept
	{
		return static_cast<RenderPhaseMask>(
			(std::uint32_t{1} << static_cast<unsigned>(RenderPhase::Count)) - 1);
	}

	[[nodiscard]] constexpr std::string_view renderPhaseName(RenderPhase p_phase) noexcept
	{
		switch (p_phase)
		{
		case RenderPhase::PassSetup:
			return "PassSetup";
		case RenderPhase::ShadowCaster:
			return "ShadowCaster";
		case RenderPhase::SceneOpaque:
			return "SceneOpaque";
		case RenderPhase::SceneTransparent:
			return "SceneTransparent";
		case RenderPhase::SceneOverlay:
			return "SceneOverlay";
		default:
			return "Invalid";
		}
	}
}

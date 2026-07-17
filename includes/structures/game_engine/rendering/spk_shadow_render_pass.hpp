#pragma once

#include <cstdint>
#include <string_view>

#include "structures/graphics/rendering/pass/spk_render_pass.hpp"
#include "structures/math/spk_matrix.hpp"

namespace spk::LightingRenderPasses
{
	inline constexpr std::string_view DirectionalShadow = "spk.lighting.directional_shadow";
}

namespace spk
{
	class ShadowRenderPass : public spk::RenderPass
	{
	private:
		std::uint32_t _cascadeIndex = 0;
		spk::Matrix4x4 _lightViewProjection;

	public:
		ShadowRenderPass(
			std::string p_id,
			std::int32_t p_priority,
			spk::RenderPass::Description p_description,
			std::uint32_t p_cascadeIndex,
			spk::Matrix4x4 p_lightViewProjection) :
			spk::RenderPass(std::move(p_id), p_priority, std::move(p_description)),
			_cascadeIndex(p_cascadeIndex),
			_lightViewProjection(std::move(p_lightViewProjection))
		{
			if (description().clear.color.has_value())
			{
				throw std::invalid_argument("ShadowRenderPass cannot clear a color attachment");
			}
		}

		[[nodiscard]] std::uint32_t cascadeIndex() const noexcept
		{
			return _cascadeIndex;
		}

		[[nodiscard]] const spk::Matrix4x4 &lightViewProjection() const noexcept
		{
			return _lightViewProjection;
		}
	};
}

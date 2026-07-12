#pragma once

#include <cstdint>

#include "structures/graphics/rendering/pass/spk_render_pass.hpp"
#include "structures/math/spk_matrix.hpp"

namespace spk::LightingRenderPasses
{
	inline constexpr auto DirectionalShadow = spk::makeRenderPassTypeId("spk.lighting.directional_shadow");
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
			spk::RenderPass::Key p_key,
			std::int32_t p_priority,
			std::size_t p_declarationOrder,
			std::string p_debugName,
			spk::RenderPass::Description p_description,
			std::uint32_t p_cascadeIndex,
			spk::Matrix4x4 p_lightViewProjection) :
			spk::RenderPass(p_key, p_priority, p_declarationOrder, std::move(p_debugName), std::move(p_description)),
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

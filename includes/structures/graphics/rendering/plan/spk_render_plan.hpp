#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "structures/graphics/rendering/pass/spk_render_pass.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit.hpp"

namespace spk
{
	struct RenderPassDiagnostics
	{
		spk::RenderPass::Key key;
		std::string debugName;
		std::string concreteType;
		std::int32_t priority = 0;
		std::size_t declarationOrder = 0;
		bool defaultTarget = false;
		bool activeTarget = false;
		spk::Viewport viewport;
		spk::RenderPassClear clear;
		std::size_t contributorCount = 0;
		std::size_t commandCount = 0;
	};

	[[nodiscard]] spk::RenderPassDiagnostics makeRenderPassDiagnostics(const spk::RenderPass &p_pass);

	class RenderPlan
	{
	private:
		std::vector<std::unique_ptr<spk::RenderPass>> _passes;
		bool _compiled = false;

		explicit RenderPlan(std::vector<std::unique_ptr<spk::RenderPass>> p_passes);
		friend class RenderPassBucketPack;

	public:
		RenderPlan() = default;
		RenderPlan(const RenderPlan &) = delete;
		RenderPlan &operator=(const RenderPlan &) = delete;
		RenderPlan(RenderPlan &&p_other) noexcept;
		RenderPlan &operator=(RenderPlan &&p_other) noexcept;

		[[nodiscard]] const std::vector<std::unique_ptr<spk::RenderPass>> &passes() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] std::vector<spk::RenderPassDiagnostics> diagnostics() const;
		[[nodiscard]] spk::RenderUnit compile();
	};
}

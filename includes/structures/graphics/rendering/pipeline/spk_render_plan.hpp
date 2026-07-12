#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "structures/graphics/rendering/pipeline/spk_render_pass.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit.hpp"

namespace spk
{
	struct RenderPassDiagnostics
	{
		RenderPassId id;
		bool defaultTarget = false;
		RenderPassClear clear;
		std::vector<RenderPhaseDiagnostics> phases;
	};

	class RenderPlan
	{
		friend class RenderPipelineBuilder;

	private:
		std::vector<RenderPass> _passes;
		bool _compiled = false;

	public:
		RenderPlan() = default;
		RenderPlan(const RenderPlan &) = delete;
		RenderPlan &operator=(const RenderPlan &) = delete;
		RenderPlan(RenderPlan &&p_other) noexcept;
		RenderPlan &operator=(RenderPlan &&p_other) noexcept;

		void addPass(RenderPass p_pass);
		[[nodiscard]] const std::vector<RenderPass> &passes() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] std::vector<RenderPassDiagnostics> diagnostics() const;
		[[nodiscard]] spk::RenderUnit compile();
	};
}

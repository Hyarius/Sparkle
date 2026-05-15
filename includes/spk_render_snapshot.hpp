#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "spk_render_unit.hpp"

namespace spk
{
	class RenderSnapshot
	{
	private:
		std::vector<std::shared_ptr<spk::RenderUnit>> _units;

	public:
		explicit RenderSnapshot(std::vector<std::shared_ptr<spk::RenderUnit>>&& p_units = {});
		~RenderSnapshot() = default;

		RenderSnapshot(const RenderSnapshot&) = delete;
		RenderSnapshot& operator=(const RenderSnapshot&) = delete;

		RenderSnapshot(RenderSnapshot&&) noexcept = default;
		RenderSnapshot& operator=(RenderSnapshot&&) noexcept = default;

		[[nodiscard]] bool empty() const;
		[[nodiscard]] size_t size() const;
		[[nodiscard]] const std::vector<std::shared_ptr<spk::RenderUnit>>& units() const;

		void execute(spk::IRenderContext& p_renderContext);
	};
}

#pragma once

#include <memory>
#include <vector>

#include "spk_render_snapshot.hpp"

namespace spk
{
	class RenderSnapshotBuilder
	{
	private:
		std::vector<std::shared_ptr<spk::RenderUnit>> _units;

	public:
		RenderSnapshotBuilder() = default;
		~RenderSnapshotBuilder() = default;

		RenderSnapshotBuilder(const RenderSnapshotBuilder&) = delete;
		RenderSnapshotBuilder& operator=(const RenderSnapshotBuilder&) = delete;

		RenderSnapshotBuilder(RenderSnapshotBuilder&&) noexcept = default;
		RenderSnapshotBuilder& operator=(RenderSnapshotBuilder&&) noexcept = default;

		void clear();
		void append(const std::shared_ptr<spk::RenderUnit>& p_unit);

		[[nodiscard]] bool empty() const;
		[[nodiscard]] size_t size() const;
		[[nodiscard]] const std::vector<std::shared_ptr<spk::RenderUnit>>& units() const;

		[[nodiscard]] spk::RenderSnapshot build();
	};
}

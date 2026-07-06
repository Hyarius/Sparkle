#pragma once

#include "core/registry.hpp"
#include "feats/feat_board.hpp"

#include <unordered_map>

namespace pg
{
	struct FeatNodeLocation
	{
		const FeatBoard *board = nullptr;
		const FeatNode *node = nullptr;
	};

	class FeatRegistry
	{
	private:
		Registry<FeatBoard> _boards;
		std::unordered_map<spk::UUID, FeatNodeLocation> _nodes;

	public:
		void load(const std::filesystem::path &p_directory, const Registry<Ability> &p_abilities, const Registry<Status> &p_statuses);
		[[nodiscard]] const FeatBoard &get(const std::string &p_id) const;
		[[nodiscard]] const FeatBoard *tryGet(const std::string &p_id) const noexcept;
		[[nodiscard]] const FeatNodeLocation *findNode(const spk::UUID &p_uuid) const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] std::size_t nodeCount() const noexcept;
		[[nodiscard]] std::vector<std::string> ids() const;
	};
}

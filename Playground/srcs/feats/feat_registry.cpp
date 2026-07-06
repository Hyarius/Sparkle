#include "feats/feat_registry.hpp"

#include "abilities/ability.hpp"
#include "statuses/status.hpp"

namespace pg
{
	void FeatRegistry::load(const std::filesystem::path &p_directory, const Registry<Ability> &p_abilities, const Registry<Status> &p_statuses)
	{
		Registry<FeatBoard> loaded;
		loaded.load(p_directory, [&](JsonReader &p_reader) { return parseFeatBoard(p_reader, p_abilities, p_statuses); });
		std::unordered_map<spk::UUID, FeatNodeLocation> nodes;
		for (const std::string &boardId : loaded.ids())
		{
			const FeatBoard &board = loaded.get(boardId);
			for (const FeatNode &node : board.nodes)
			{
				if (!nodes.emplace(node.uuid, FeatNodeLocation{&board, &node}).second)
					throw JsonError(p_directory / (boardId + ".json"), "$.nodes", "node UUID '" + node.uuid.toString() + "' is already used by another board");
			}
		}
		_boards = std::move(loaded); _nodes = std::move(nodes);
	}

	const FeatBoard &FeatRegistry::get(const std::string &p_id) const { return _boards.get(p_id); }
	const FeatBoard *FeatRegistry::tryGet(const std::string &p_id) const noexcept { return _boards.tryGet(p_id); }
	const FeatNodeLocation *FeatRegistry::findNode(const spk::UUID &p_uuid) const noexcept
	{
		const auto found = _nodes.find(p_uuid); return found == _nodes.end() ? nullptr : &found->second;
	}
	std::size_t FeatRegistry::size() const noexcept { return _boards.size(); }
	std::size_t FeatRegistry::nodeCount() const noexcept { return _nodes.size(); }
	std::vector<std::string> FeatRegistry::ids() const { return _boards.ids(); }
}

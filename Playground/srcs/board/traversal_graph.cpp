#include "board/traversal_graph.hpp"

#include <stdexcept>

namespace pg
{
	std::size_t TraversalGraph::addNode(const spk::Vector3Int &p_position)
	{
		if (const auto found = _index.find(p_position); found != _index.end()) return found->second;
		const std::size_t index = _nodes.size();
		_nodes.push_back({.position = p_position});
		_index.emplace(p_position, index);
		return index;
	}

	void TraversalGraph::connect(std::size_t p_from, std::size_t p_directionIndex, std::size_t p_to)
	{
		_nodes.at(p_from).neighbors.at(p_directionIndex) = p_to;
	}

	const TraversalGraph::Node *TraversalGraph::tryGetNode(const spk::Vector3Int &p_position) const noexcept
	{
		const auto found = _index.find(p_position);
		return found == _index.end() ? nullptr : &_nodes[found->second];
	}

	const TraversalGraph::Node &TraversalGraph::node(std::size_t p_index) const
	{
		return _nodes.at(p_index);
	}

	std::optional<std::size_t> TraversalGraph::indexOf(const spk::Vector3Int &p_position) const noexcept
	{
		const auto found = _index.find(p_position);
		return found == _index.end() ? std::nullopt : std::optional<std::size_t>(found->second);
	}

	const std::vector<TraversalGraph::Node> &TraversalGraph::allNodes() const noexcept { return _nodes; }
	std::size_t TraversalGraph::size() const noexcept { return _nodes.size(); }
}

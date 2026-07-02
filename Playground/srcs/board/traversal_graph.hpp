#pragma once

#include "structures/math/spk_vector3.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <tuple>
#include <vector>

namespace pg
{
	struct CellPositionLess
	{
		[[nodiscard]] bool operator()(const spk::Vector3Int &p_left, const spk::Vector3Int &p_right) const noexcept
		{
			return std::tie(p_left.x, p_left.y, p_left.z) < std::tie(p_right.x, p_right.y, p_right.z);
		}
	};

	class TraversalGraph
	{
	public:
		struct Node
		{
			spk::Vector3Int position{};
			std::array<std::optional<std::size_t>, 4> neighbors{};
		};

	private:
		std::vector<Node> _nodes;
		std::map<spk::Vector3Int, std::size_t, CellPositionLess> _index;

	public:
		[[nodiscard]] std::size_t addNode(const spk::Vector3Int &p_position);
		void connect(std::size_t p_from, std::size_t p_directionIndex, std::size_t p_to);
		[[nodiscard]] const Node *tryGetNode(const spk::Vector3Int &p_position) const noexcept;
		[[nodiscard]] const Node &node(std::size_t p_index) const;
		[[nodiscard]] std::optional<std::size_t> indexOf(const spk::Vector3Int &p_position) const noexcept;
		[[nodiscard]] const std::vector<Node> &allNodes() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
	};
}

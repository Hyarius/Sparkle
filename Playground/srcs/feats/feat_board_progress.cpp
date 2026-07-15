#include "feats/feat_board_progress.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <variant>

namespace pg
{
	namespace
	{
		// Depth first, definition order: the condition itself, then every nested child. A
		// composite keeps an entry of its own, so "completed" means the composite's own rule
		// (all / any of its children) held, not merely that some child did.
		void flattenCondition(
			const ConditionSpec &p_condition,
			std::vector<PersistentConditionAdvancement> &p_advancement)
		{
			PersistentConditionAdvancement entry;
			entry.conditionId = p_condition.id;
			p_advancement.push_back(std::move(entry));

			if (const auto *allOf = std::get_if<AllOfConditionSpec>(&p_condition.payload))
			{
				for (const ConditionSpec &child : allOf->children)
				{
					flattenCondition(child, p_advancement);
				}
			}
			else if (const auto *anyOf = std::get_if<AnyOfConditionSpec>(&p_condition.payload))
			{
				for (const ConditionSpec &child : anyOf->children)
				{
					flattenCondition(child, p_advancement);
				}
			}
		}

		[[nodiscard]] bool changesForm(const FeatNodeDefinition &p_node) noexcept
		{
			return std::ranges::any_of(p_node.rewards, [](const FeatRewardSpec &p_reward) {
				return std::holds_alternative<ChangeFormRewardSpec>(p_reward.payload);
			});
		}

		[[nodiscard]] const std::string &changeFormTarget(const FeatNodeDefinition &p_node)
		{
			for (const FeatRewardSpec &reward : p_node.rewards)
			{
				if (const auto *changeForm = std::get_if<ChangeFormRewardSpec>(&reward.payload))
				{
					return changeForm->form;
				}
			}
			throw std::invalid_argument("node '" + p_node.id + "' changes no form");
		}

		[[noreturn]] void fail(const FeatBoardDefinition &p_board, const std::string &p_message)
		{
			throw std::invalid_argument("board '" + p_board.id + "' " + p_message);
		}

		// A working copy of the one legal-order search the preset function performs.
		class PresetSolver
		{
		private:
			const FeatBoardDefinition &_board;
			std::set<std::string> _remaining;
			std::set<std::string> _accepted;
			std::string _form;
			std::size_t _acceptedNonRoot = 0;

			[[nodiscard]] bool _hasAcceptedNeighbour(const FeatNodeDefinition &p_node) const
			{
				return std::ranges::any_of(p_node.neighbours, [this](const std::string &p_neighbour) {
					return _accepted.contains(p_neighbour);
				});
			}

			[[nodiscard]] bool _isEligible(const FeatNodeDefinition &p_node) const
			{
				return _hasAcceptedNeighbour(p_node) && _acceptedNonRoot >= p_node.minimumCompletedNodes &&
					   (!p_node.fromForm.has_value() || *p_node.fromForm == _form);
			}

			void _accept(const FeatNodeDefinition &p_node)
			{
				_accepted.insert(p_node.id);
				_remaining.erase(p_node.id);
				++_acceptedNonRoot;
				if (changesForm(p_node))
				{
					_form = changeFormTarget(p_node);
				}
			}

		public:
			PresetSolver(const FeatBoardDefinition &p_board, std::set<std::string> p_completed, std::string p_startingForm) :
				_board(p_board),
				_remaining(std::move(p_completed)),
				_form(std::move(p_startingForm))
			{
				_accepted.insert(p_board.rootNodeId);
			}

			// Completing a node never un-gates another - the count only grows and a form only
			// climbs - so a greedy pass is complete as long as it delays the irreversible choice:
			// take every eligible ordinary node first, and only then one evolution.
			void solve()
			{
				bool progressed = true;
				while (progressed)
				{
					progressed = false;
					for (const FeatNodeDefinition &node : _board.nodes)
					{
						if (_remaining.contains(node.id) && !changesForm(node) && _isEligible(node))
						{
							_accept(node);
							progressed = true;
						}
					}
					if (progressed)
					{
						continue;
					}
					for (const FeatNodeDefinition &node : _board.nodes)
					{
						if (_remaining.contains(node.id) && changesForm(node) && _isEligible(node))
						{
							_accept(node);
							progressed = true;
							break;
						}
					}
				}
			}

			// The first blocked node in definition order, with the gate that blocked it: an author
			// reading the message must not have to re-derive the whole order.
			[[noreturn]] void failOnBlockedNode() const
			{
				for (const FeatNodeDefinition &node : _board.nodes)
				{
					if (!_remaining.contains(node.id))
					{
						continue;
					}
					if (!_hasAcceptedNeighbour(node))
					{
						fail(
							_board,
							"completes node '" + node.id + "', which no completed node is adjacent to");
					}
					if (_acceptedNonRoot < node.minimumCompletedNodes)
					{
						fail(
							_board,
							"completes node '" + node.id + "', which needs " +
								std::to_string(node.minimumCompletedNodes) + " completed nodes and can reach only " +
								std::to_string(_acceptedNonRoot));
					}
					fail(
						_board,
						"completes node '" + node.id + "', which needs form '" + node.fromForm.value_or("") +
							"' and can only reach '" + _form + "'");
				}
				fail(_board, "reported a blocked preset with nothing blocked");
			}

			[[nodiscard]] bool solved() const noexcept
			{
				return _remaining.empty();
			}
		};
	}

	const FeatNodeProgress *FeatBoardProgress::tryNode(std::string_view p_nodeId) const noexcept
	{
		const auto entry = std::ranges::find(nodes, p_nodeId, &FeatNodeProgress::nodeId);
		return entry == nodes.end() ? nullptr : &*entry;
	}

	bool FeatBoardProgress::isCompleted(std::string_view p_nodeId) const noexcept
	{
		const FeatNodeProgress *node = tryNode(p_nodeId);
		return node != nullptr && node->completed;
	}

	FeatBoardProgress makeFreshFeatBoardProgress(const FeatBoardDefinition &p_board)
	{
		FeatBoardProgress result;
		result.boardId = p_board.id;
		result.nodes.reserve(p_board.nodes.size());

		for (const FeatNodeDefinition &node : p_board.nodes)
		{
			FeatNodeProgress progress;
			progress.nodeId = node.id;
			// The root has no requirement, so it is complete the moment the creature exists. Its
			// rewards replay from the baseline like any other node's.
			progress.completed = node.kind == FeatNodeKind::Root;
			progress.requirements.reserve(node.requirements.size());

			for (const ConditionSpec &requirement : node.requirements)
			{
				FeatRequirementProgress entry;
				entry.requirementId = requirement.id;
				flattenCondition(requirement, entry.conditionAdvancement);
				progress.requirements.push_back(std::move(entry));
			}
			result.nodes.push_back(std::move(progress));
		}
		return result;
	}

	FeatBoardProgress makePresetFeatBoardProgress(
		const FeatBoardDefinition &p_board,
		std::string_view p_startingFormId,
		std::span<const std::string> p_completedNodeIds)
	{
		FeatBoardProgress result = makeFreshFeatBoardProgress(p_board);

		std::set<std::string> completed;
		for (const std::string &nodeId : p_completedNodeIds)
		{
			const FeatNodeProgress *node = result.tryNode(nodeId);
			if (node == nullptr)
			{
				fail(p_board, "has no node '" + nodeId + "'");
			}
			if (nodeId == p_board.rootNodeId)
			{
				fail(p_board, "starts with its root completed; '" + nodeId + "' must not be listed");
			}
			if (!completed.insert(nodeId).second)
			{
				fail(p_board, "lists node '" + nodeId + "' twice");
			}
		}

		// Rebuilt from the completed nodes rather than read from the author, so a contradictory
		// map can never claim a branch the creature did not take.
		for (const FeatNodeDefinition &node : p_board.nodes)
		{
			if (!completed.contains(node.id) || !node.exclusiveGroup.has_value())
			{
				continue;
			}
			const auto [entry, inserted] = result.chosenExclusiveGroups.emplace(*node.exclusiveGroup, node.id);
			if (!inserted)
			{
				fail(
					p_board,
					"completes both '" + entry->second + "' and '" + node.id + "', which are exclusive members of '" +
						*node.exclusiveGroup + "'");
			}
		}

		PresetSolver solver(p_board, completed, std::string(p_startingFormId));
		solver.solve();
		if (!solver.solved())
		{
			solver.failOnBlockedNode();
		}

		// Only the node completes. Its condition advancement stays at zero on purpose: a completed
		// node is never re-evaluated, and writing "completed" into the children of an anyOf the
		// enemy never actually played would be a fiction the save then carries forever.
		for (FeatNodeProgress &node : result.nodes)
		{
			if (completed.contains(node.nodeId))
			{
				node.completed = true;
			}
		}
		return result;
	}
}

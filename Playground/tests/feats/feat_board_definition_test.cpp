#include <gtest/gtest.h>

#include "feats/feat_board_definition.hpp"
#include "support/json_fixture.hpp"

#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace
{
	// One movement requirement, so a non-root node can be authored in one line.
	[[nodiscard]] std::string requirement(std::string_view p_id)
	{
		return std::string(R"({"id": ")") + std::string(p_id) + R"(", "type": "movement",
			"descriptionKey": "feat.fixture.description", "window": "fight", "windowActor": "any",
			"requiredWindowCount": 1, "windowMode": "cumulative", "actor": "subject", "movement": "voluntary",
			"direction": "any", "aggregation": "sum", "comparison": "atLeast", "threshold": 10})";
	}

	[[nodiscard]] std::string unlockAbility(std::string_view p_rewardId, std::string_view p_abilityId)
	{
		return std::string(R"({"id": ")") + std::string(p_rewardId) + R"(", "type": "unlockAbility", "ability": ")" +
			   std::string(p_abilityId) + R"("})";
	}

	[[nodiscard]] std::string changeForm(std::string_view p_rewardId, std::string_view p_formId)
	{
		return std::string(R"({"id": ")") + std::string(p_rewardId) + R"(", "type": "changeForm", "form": ")" +
			   std::string(p_formId) + R"("})";
	}

	struct Node
	{
		std::string id;
		std::string kind = "stats";
		std::string neighbours = "[]";
		std::string requirements = "[]";
		std::string rewards = "[]";
		int minimumCompletedNodes = 0;
		// Empty means the optional key is omitted, which is the only way to say "absent": an
		// explicit null is a type error in schema version 1.
		std::string exclusiveGroup;
		std::string fromForm;
		// Raw extra keys, for the cases that author something the schema must reject.
		std::string extra;

		[[nodiscard]] std::string json() const
		{
			std::string result = R"({"id": ")" + id + R"(", "displayNameKey": "feat.fixture.name",
				"descriptionKey": "feat.fixture.description", "position": [0, 0], "kind": ")" +
								 kind +
								 R"(", "neighbours": )" + neighbours + R"(, "minimumCompletedNodes": )" +
								 std::to_string(minimumCompletedNodes);
			if (!exclusiveGroup.empty())
			{
				result += R"(, "exclusiveGroup": ")" + exclusiveGroup + R"(")";
			}
			if (!fromForm.empty())
			{
				result += R"(, "fromForm": ")" + fromForm + R"(")";
			}
			result += R"(, "requirements": )" + requirements + R"(, "rewards": )" + rewards + extra + "}";
			return result;
		}
	};

	[[nodiscard]] std::string board(const std::vector<Node> &p_nodes, std::string_view p_rootNode = "root")
	{
		std::string nodes;
		for (const Node &node : p_nodes)
		{
			if (!nodes.empty())
			{
				nodes += ",";
			}
			nodes += node.json();
		}
		return std::string(R"({"version": 1, "displayNameKey": "feat-board.fixture.name", "rootNode": ")") +
			   std::string(p_rootNode) + R"(", "nodes": [)" + nodes + "]}";
	}

	[[nodiscard]] Node rootNode(std::string_view p_neighbours)
	{
		Node node;
		node.id = "root";
		node.kind = "root";
		node.neighbours = std::string(p_neighbours);
		return node;
	}

	[[nodiscard]] pg::FeatBoardDefinition parse(const std::string &p_json)
	{
		const pgtest::Document document(p_json, "training-board.json");
		pg::JsonReader reader = document.reader();
		return pg::parseFeatBoardDefinition(reader, pg::ConditionLimits{});
	}

	// The smallest legal board: a root and one earned ability node.
	[[nodiscard]] std::string twoNodeBoard()
	{
		Node guard;
		guard.id = "learn-guard";
		guard.kind = "ability";
		guard.neighbours = R"(["root"])";
		guard.requirements = "[" + requirement("move-ten") + "]";
		guard.rewards = "[" + unlockAbility("unlock-guard", "training-guard") + "]";
		return board({rootNode(R"(["learn-guard"])"), guard});
	}
}

TEST(FeatBoardDefinitionTest, ParsesAConnectedBoardAndKeepsItsAuthoredOrder)
{
	Node guard;
	guard.id = "learn-guard";
	guard.kind = "ability";
	guard.neighbours = R"(["root", "learn-snare"])";
	guard.requirements = "[" + requirement("take-damage") + "]";
	guard.rewards = "[" + unlockAbility("unlock-guard", "training-guard") + "]";

	Node snare;
	snare.id = "learn-snare";
	snare.kind = "ability";
	snare.neighbours = R"(["learn-guard"])";
	snare.minimumCompletedNodes = 1;
	snare.requirements = "[" + requirement("move-ten") + "]";
	snare.rewards = "[" + unlockAbility("unlock-snare", "training-snare") + "]";

	const pg::FeatBoardDefinition definition = parse(board({rootNode(R"(["learn-guard"])"), guard, snare}));

	EXPECT_EQ(definition.displayNameKey, "feat-board.fixture.name");
	EXPECT_EQ(definition.rootNodeId, "root");
	ASSERT_EQ(definition.nodes.size(), 3U);

	// Definition order, not traversal order, is authoritative: reward replay and tie-breaks read
	// this vector.
	EXPECT_EQ(definition.nodes[0].id, "root");
	EXPECT_EQ(definition.nodes[1].id, "learn-guard");
	EXPECT_EQ(definition.nodes[2].id, "learn-snare");

	EXPECT_EQ(definition.nodes[0].kind, pg::FeatNodeKind::Root);
	EXPECT_TRUE(definition.nodes[0].requirements.empty()) << "the root is the only node that may have none";
	EXPECT_EQ(definition.nodes[2].minimumCompletedNodes, 1U);
	EXPECT_FALSE(definition.nodes[2].fromForm.has_value());
	EXPECT_FALSE(definition.nodes[2].exclusiveGroup.has_value());

	ASSERT_EQ(definition.nodes[1].requirements.size(), 1U);
	EXPECT_EQ(definition.nodes[1].requirements[0].id, "take-damage");
	ASSERT_EQ(definition.nodes[1].rewards.size(), 1U);
	EXPECT_EQ(definition.nodes[1].rewards[0].id, "unlock-guard");
	EXPECT_EQ(definition.nodes[1].source.jsonPath, "$.nodes[1]");
}

TEST(FeatBoardDefinitionTest, EnforcesTheRootContract)
{
	EXPECT_NO_THROW(auto value = parse(twoNodeBoard()));

	Node earned;
	earned.id = "learn-guard";
	earned.kind = "ability";
	earned.neighbours = R"(["root"])";
	earned.requirements = "[" + requirement("move-ten") + "]";
	earned.rewards = "[" + unlockAbility("unlock-guard", "training-guard") + "]";

	// rootNode must name the one node of kind root.
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), earned}, "learn-guard")), pg::JsonError);
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), earned}, "ghost")), pg::JsonError);

	Node secondRoot;
	secondRoot.id = "learn-guard";
	secondRoot.kind = "root";
	secondRoot.neighbours = R"(["root"])";
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), secondRoot})), pg::JsonError) << "a board has one root";

	Node noRequirement;
	noRequirement.id = "learn-guard";
	noRequirement.kind = "ability";
	noRequirement.neighbours = R"(["root"])";
	noRequirement.rewards = "[" + unlockAbility("unlock-guard", "training-guard") + "]";
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), noRequirement})), pg::JsonError)
		<< "every node but the root has to be earned";

	// The root starts completed, so it can gate on nothing.
	Node gatedRoot = rootNode(R"(["learn-guard"])");
	gatedRoot.minimumCompletedNodes = 1;
	EXPECT_THROW(auto value = parse(board({gatedRoot, earned})), pg::JsonError);

	Node formedRoot = rootNode(R"(["learn-guard"])");
	formedRoot.fromForm = "bloom";
	EXPECT_THROW(auto value = parse(board({formedRoot, earned})), pg::JsonError);

	Node exclusiveRoot = rootNode(R"(["learn-guard"])");
	exclusiveRoot.exclusiveGroup = "branch";
	EXPECT_THROW(auto value = parse(board({exclusiveRoot, earned})), pg::JsonError);
}

TEST(FeatBoardDefinitionTest, EnforcesTheAdjacencyContract)
{
	const auto boardWithNeighbours = [](std::string_view p_rootNeighbours, std::string_view p_nodeNeighbours) {
		Node earned;
		earned.id = "learn-guard";
		earned.kind = "ability";
		earned.neighbours = std::string(p_nodeNeighbours);
		earned.requirements = "[" + requirement("move-ten") + "]";
		earned.rewards = "[" + unlockAbility("unlock-guard", "training-guard") + "]";
		return board({rootNode(p_rootNeighbours), earned});
	};

	EXPECT_THROW(auto value = parse(boardWithNeighbours(R"(["ghost"])", R"(["root"])")), pg::JsonError) << "unknown neighbour";
	EXPECT_THROW(auto value = parse(boardWithNeighbours(R"(["root"])", R"(["root"])")), pg::JsonError) << "self edge";
	EXPECT_THROW(
		auto value = parse(boardWithNeighbours(R"(["learn-guard", "learn-guard"])", R"(["root"])")),
		pg::JsonError)
		<< "duplicate neighbour";
	EXPECT_THROW(auto value = parse(boardWithNeighbours(R"(["learn-guard"])", "[]")), pg::JsonError)
		<< "an edge authored on one side only is not an edge";
	EXPECT_THROW(auto value = parse(boardWithNeighbours("[]", "[]")), pg::JsonError)
		<< "a node no path reaches from the root can never activate";
}

TEST(FeatBoardDefinitionTest, EnforcesUniqueNodeConditionAndRewardIds)
{
	Node first;
	first.id = "learn-guard";
	first.kind = "ability";
	first.neighbours = R"(["root", "learn-snare"])";
	first.requirements = "[" + requirement("move-ten") + "]";
	first.rewards = "[" + unlockAbility("unlock-guard", "training-guard") + "]";

	Node second;
	second.id = "learn-snare";
	second.kind = "ability";
	second.neighbours = R"(["learn-guard"])";
	second.requirements = "[" + requirement("move-ten") + "]";
	second.rewards = "[" + unlockAbility("unlock-snare", "training-snare") + "]";
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), first, second})), pg::JsonError)
		<< "condition ids are unique across the whole board, not per node";

	second.requirements = "[" + requirement("chase") + "]";
	second.rewards = "[" + unlockAbility("unlock-guard", "training-snare") + "]";
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), first, second})), pg::JsonError)
		<< "reward ids are unique across the whole board";

	second.rewards = "[" + unlockAbility("unlock-snare", "training-snare") + "]";
	EXPECT_NO_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), first, second})));

	Node original;
	original.id = "learn-guard";
	original.kind = "ability";
	original.neighbours = R"(["root"])";
	original.requirements = "[" + requirement("move-ten") + "]";
	original.rewards = "[" + unlockAbility("unlock-guard", "training-guard") + "]";

	Node duplicate = original;
	duplicate.requirements = "[" + requirement("chase") + "]";
	duplicate.rewards = "[" + unlockAbility("unlock-chase", "training-snare") + "]";
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), original, duplicate})), pg::JsonError)
		<< "duplicate node id";
}

TEST(FeatBoardDefinitionTest, RejectsStaleRepeatFieldsAndUnsupportedVersions)
{
	Node repeatable;
	repeatable.id = "learn-guard";
	repeatable.kind = "ability";
	repeatable.neighbours = R"(["root"])";
	repeatable.requirements = "[" + requirement("move-ten") + "]";
	repeatable.rewards = "[" + unlockAbility("unlock-guard", "training-guard") + "]";
	repeatable.extra = R"(, "repeatLimit": 3)";

	// Nodes are one-time completions: there is no repeat model to reinterpret this into.
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), repeatable})), pg::JsonError);

	repeatable.extra = R"(, "repeatable": true)";
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), repeatable})), pg::JsonError);

	std::string wrongVersion = twoNodeBoard();
	wrongVersion.replace(wrongVersion.find(R"("version": 1)"), 12, R"("version": 2)");
	EXPECT_THROW(auto value = parse(wrongVersion), pg::JsonError);
}

TEST(FeatBoardDefinitionTest, BoundsTheCompletedNodeGate)
{
	Node gated;
	gated.id = "learn-guard";
	gated.kind = "ability";
	gated.neighbours = R"(["root"])";
	gated.requirements = "[" + requirement("move-ten") + "]";
	gated.rewards = "[" + unlockAbility("unlock-guard", "training-guard") + "]";

	gated.minimumCompletedNodes = 1;
	EXPECT_NO_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), gated})));

	// A gate on more nodes than the board has could never open.
	gated.minimumCompletedNodes = 2;
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), gated})), pg::JsonError);

	gated.minimumCompletedNodes = -1;
	EXPECT_THROW(auto value = parse(board({rootNode(R"(["learn-guard"])"), gated})), pg::JsonError);
}

TEST(FeatBoardDefinitionTest, EnforcesKindAndRewardCoherence)
{
	const auto singleNodeBoard = [](std::string_view p_kind, const std::string &p_rewards) {
		Node node;
		node.id = "node";
		node.kind = std::string(p_kind);
		node.neighbours = R"(["root"])";
		node.requirements = "[" + requirement("move-ten") + "]";
		node.rewards = p_rewards;
		return board({rootNode(R"(["node"])"), node});
	};

	const std::string bonusStat = R"([{"id": "tougher", "type": "bonusStat", "stat": "armor", "amount": 2}])";
	const std::string passive = R"([{"id": "thorny", "type": "unlockPassive", "status": "thorns"}])";

	EXPECT_NO_THROW(auto value = parse(singleNodeBoard("stats", bonusStat)));
	EXPECT_THROW(auto value = parse(singleNodeBoard("stats", passive)), pg::JsonError);

	EXPECT_NO_THROW(auto value = parse(singleNodeBoard("ability", "[" + unlockAbility("learn", "training-guard") + "]")));
	EXPECT_NO_THROW(auto value = parse(singleNodeBoard("ability", R"([{"id": "forget", "type": "removeAbility", "ability": "training-strike"}])")));
	EXPECT_THROW(auto value = parse(singleNodeBoard("ability", bonusStat)), pg::JsonError);

	EXPECT_NO_THROW(auto value = parse(singleNodeBoard("passive", passive)));
	EXPECT_THROW(auto value = parse(singleNodeBoard("passive", bonusStat)), pg::JsonError);

	// Extra compatible rewards stay legal: a meaningful node may grant more than one result.
	EXPECT_NO_THROW(auto value = parse(singleNodeBoard("ability", "[" + unlockAbility("learn", "training-guard") + R"(, {"id": "tougher", "type": "bonusStat", "stat": "armor", "amount": 2}])")));

	// Only an evolution node changes form, and it changes it exactly once.
	EXPECT_THROW(auto value = parse(singleNodeBoard("stats", "[" + changeForm("bloom", "bloom") + "]")), pg::JsonError);
	EXPECT_THROW(auto value = parse(singleNodeBoard("evolution", bonusStat)), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(singleNodeBoard(
			"evolution",
			"[" + changeForm("to-bloom", "bloom") + "," + changeForm("to-thorn", "thorn") + "]")),
		pg::JsonError);
	EXPECT_NO_THROW(auto value = parse(singleNodeBoard("evolution", "[" + changeForm("to-bloom", "bloom") + "]")));
}

TEST(FeatBoardDefinitionTest, EnforcesExclusiveEvolutionGroups)
{
	const auto evolutionBoard = [](const Node &p_first, const Node &p_second) {
		return board({rootNode(R"(["bloom", "thorn"])"), p_first, p_second});
	};

	const auto branch = [](std::string_view p_id, std::string_view p_form, std::string_view p_kind = "evolution") {
		Node node;
		node.id = std::string(p_id);
		node.kind = std::string(p_kind);
		node.neighbours = R"(["root"])";
		node.exclusiveGroup = "first-evolution";
		node.requirements = "[" + requirement(std::string("earn-") + std::string(p_id)) + "]";
		node.rewards = "[" + changeForm(std::string("to-") + std::string(p_id), p_form) + "]";
		return node;
	};

	const pg::FeatBoardDefinition definition = parse(evolutionBoard(branch("bloom", "bloom"), branch("thorn", "thorn")));
	ASSERT_EQ(definition.nodes.size(), 3U);
	ASSERT_TRUE(definition.nodes[1].exclusiveGroup.has_value());
	EXPECT_EQ(*definition.nodes[1].exclusiveGroup, "first-evolution");
	// Completing one member permanently blocks the other; when both qualify in the same battle,
	// earlier definition order wins.
	EXPECT_EQ(definition.nodes[1].id, "bloom");

	// A group of one excludes nothing.
	Node lonely = branch("bloom", "bloom");
	Node ungrouped = branch("thorn", "thorn");
	ungrouped.exclusiveGroup.clear();
	EXPECT_THROW(auto value = parse(evolutionBoard(lonely, ungrouped)), pg::JsonError);

	// Two members leading to the same form make the choice meaningless while still blocking the
	// other branch forever.
	EXPECT_THROW(auto value = parse(evolutionBoard(branch("bloom", "bloom"), branch("thorn", "bloom"))), pg::JsonError);

	// A member that is not an evolution has no form to choose between.
	Node statsMember = branch("thorn", "thorn", "stats");
	statsMember.rewards = R"([{"id": "tougher", "type": "bonusStat", "stat": "armor", "amount": 2}])";
	EXPECT_THROW(auto value = parse(evolutionBoard(branch("bloom", "bloom"), statsMember)), pg::JsonError);
}

TEST(FeatBoardDefinitionTest, DefersFormReferencesToTheSelectingSpecies)
{
	Node bloom;
	bloom.id = "bloom";
	bloom.kind = "evolution";
	bloom.neighbours = R"(["root", "bloom-strength"])";
	bloom.requirements = "[" + requirement("earn-bloom") + "]";
	bloom.rewards = "[" + changeForm("to-bloom", "bloom") + "]";

	Node gated;
	gated.id = "bloom-strength";
	gated.kind = "stats";
	gated.neighbours = R"(["bloom"])";
	gated.fromForm = "bloom";
	gated.requirements = "[" + requirement("train-bloom") + "]";
	gated.rewards = R"([{"id": "stronger", "type": "bonusStat", "stat": "strength", "amount": 2}])";

	const pg::FeatBoardDefinition definition = parse(board({rootNode(R"(["bloom"])"), bloom, gated}));

	// A board on its own does not know what a form is: the ids survive parsing with their paths.
	EXPECT_EQ(pg::featBoardFormReferences(definition), (std::set<std::string>{"bloom"}));
	ASSERT_TRUE(definition.nodes[2].fromForm.has_value());
	EXPECT_EQ(*definition.nodes[2].fromForm, "bloom");

	EXPECT_NO_THROW(pg::validateFeatBoardFormReferences(definition, {"seed", "bloom"}, "training-creature"));

	try
	{
		pg::validateFeatBoardFormReferences(definition, {"seed"}, "training-creature");
		FAIL() << "a species that has no 'bloom' form cannot select this board";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.file().filename(), std::filesystem::path("training-board.json"))
			<< "the deferred check still names the board file the author has to fix";
		EXPECT_NE(error.message().find("training-creature"), std::string::npos);
		EXPECT_NE(error.message().find("bloom"), std::string::npos);
	}
}

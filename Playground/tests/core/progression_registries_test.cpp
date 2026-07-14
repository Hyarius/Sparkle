#include <gtest/gtest.h>

#include "core/paths.hpp"
#include "core/registries.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	// A scratch copy of the shipped data whose featboards directory starts out empty, so a case
	// authors exactly the board it is about while keeping the real combat definitions to
	// reference.
	class ProgressionData
	{
	private:
		std::filesystem::path _path;

	public:
		explicit ProgressionData(std::string_view p_name) :
			_path(std::filesystem::temp_directory_path() / "sparkle-playground-progression" / p_name)
		{
			std::filesystem::remove_all(_path);
			std::filesystem::create_directories(_path);
			std::filesystem::copy(
				pg::resourceRoot() / "data",
				_path,
				std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

			for (const std::filesystem::directory_entry &entry :
				 std::filesystem::directory_iterator(_path / "featboards"))
			{
				std::filesystem::remove_all(entry.path());
			}
		}

		~ProgressionData()
		{
			std::error_code error;
			std::filesystem::remove_all(_path, error);
		}

		ProgressionData(const ProgressionData &) = delete;
		ProgressionData &operator=(const ProgressionData &) = delete;

		[[nodiscard]] const std::filesystem::path &path() const noexcept { return _path; }

		void featBoard(std::string_view p_id, std::string_view p_content) const
		{
			const std::filesystem::path target = _path / "featboards" / (std::string(p_id) + ".json");
			std::ofstream stream(target, std::ios::binary | std::ios::trunc);
			stream << p_content;
		}

		void status(std::string_view p_id, std::string_view p_content) const
		{
			const std::filesystem::path target = _path / "statuses" / (std::string(p_id) + ".json");
			std::ofstream stream(target, std::ios::binary | std::ios::trunc);
			stream << p_content;
		}
	};

	[[nodiscard]] std::string requirement(std::string_view p_id, std::string_view p_abilities)
	{
		return std::string(R"({"id": ")") + std::string(p_id) + R"(", "type": "damage",
			"descriptionKey": "feat.fixture.description", "window": "fight", "windowActor": "any",
			"requiredWindowCount": 1, "windowMode": "cumulative", "actor": "subject", "role": "source",
			"counterpart": "opponentTeam", "kind": "any", "component": "total", "targetHadShield": "any",
			"sourceAbilities": )" +
			   std::string(p_abilities) +
			   R"(, "aggregation": "sum", "comparison": "atLeast", "threshold": 10})";
	}

	// A root plus one earned node, whose requirement filter and reward the case supplies.
	[[nodiscard]] std::string boardFile(std::string_view p_requirement, std::string_view p_rewards)
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "feat-board.fixture.name",
			"rootNode": "root",
			"nodes": [
				{"id": "root", "displayNameKey": "feat.fixture.name",
				 "descriptionKey": "feat.fixture.description", "position": [0, 0], "kind": "root",
				 "neighbours": ["earned"], "minimumCompletedNodes": 0, "requirements": [], "rewards": []},
				{"id": "earned", "displayNameKey": "feat.fixture.name",
				 "descriptionKey": "feat.fixture.description", "position": [1, 0], "kind": "ability",
				 "neighbours": ["root"], "minimumCompletedNodes": 0, "requirements": [)") +
			   std::string(p_requirement) + R"(], "rewards": [)" + std::string(p_rewards) + "]}]}";
	}

	constexpr std::string_view UnlockGuard =
		R"({"id": "unlock-guard", "type": "unlockAbility", "ability": "training-guard"})";

	[[nodiscard]] std::string statusFile(std::string_view p_tags)
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "status.fixture.name",
			"descriptionKey": "fixture.description",
			"icon": [0, 0],
			"tags": )") +
			   std::string(p_tags) +
			   R"(, "stacking": {"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"},
			   "modifiers": [{"id": "armor-bonus", "stat": "armor", "operation": "add", "value": 1,
			                  "perStack": false}],
			   "hooks": []})";
	}
}

TEST(ProgressionRegistriesTest, LoadsTheShippedTrainingBoard)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	ASSERT_EQ(registries.featBoards().size(), 1U);
	ASSERT_TRUE(registries.featBoards().contains("training-board"));

	const pg::FeatBoardDefinition &board = registries.featBoards().get("training-board");
	EXPECT_EQ(board.id, "training-board") << "the id is the filename stem, assigned by the loader";
	EXPECT_EQ(board.rootNodeId, "root");

	ASSERT_EQ(board.nodes.size(), 3U);
	EXPECT_EQ(board.nodes[0].id, "root");
	EXPECT_EQ(board.nodes[1].id, "learn-guard");
	EXPECT_EQ(board.nodes[2].id, "learn-snare");
	EXPECT_EQ(board.nodes[2].minimumCompletedNodes, 1U);

	// It references the step-02 seed abilities and, deliberately, no form: it is valid before any
	// species exists.
	EXPECT_TRUE(pg::featBoardFormReferences(board).empty());
	EXPECT_TRUE(registries.abilities().contains("training-guard"));
	EXPECT_TRUE(registries.abilities().contains("training-snare"));
}

TEST(ProgressionRegistriesTest, ResolvesConditionAndRewardReferencesAgainstTheCombatRegistries)
{
	const ProgressionData known("known-references");
	known.featBoard("training", boardFile(requirement("hit-with-strike", R"(["training-strike"])"), UnlockGuard));

	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(known.path()));
	EXPECT_EQ(registries.featBoards().size(), 1U);

	const ProgressionData unknownAbilityFilter("unknown-filter");
	unknownAbilityFilter.featBoard("training", boardFile(requirement("hit", R"(["ghost-ability"])"), UnlockGuard));
	try
	{
		registries.loadAll(unknownAbilityFilter.path());
		FAIL() << "a condition that filters on an unknown ability must fail the load";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.file().filename(), std::filesystem::path("training.json"));
		EXPECT_EQ(error.path(), "$.nodes[1].requirements[0]") << "the authored path survives the second phase";
		EXPECT_NE(error.message().find("earned"), std::string::npos) << "names the owning node";
		EXPECT_NE(error.message().find("ghost-ability"), std::string::npos);
	}

	const ProgressionData unknownReward("unknown-reward");
	unknownReward.featBoard(
		"training",
		boardFile(
			requirement("hit", "[]"),
			R"({"id": "unlock-ghost", "type": "unlockAbility", "ability": "ghost-ability"})"));
	EXPECT_THROW(registries.loadAll(unknownReward.path()), pg::JsonError);

	// A status referenced from the wrong registry does not accidentally resolve.
	const ProgressionData wrongRegistry("wrong-registry");
	wrongRegistry.featBoard(
		"training",
		boardFile(
			requirement("hit", R"(["training-guarded"])"),
			UnlockGuard));
	EXPECT_THROW(registries.loadAll(wrongRegistry.path()), pg::JsonError)
		<< "'training-guarded' is a status, so it is not an ability";
}

TEST(ProgressionRegistriesTest, RejectsAStunGrantedAsAPermanentPassive)
{
	const ProgressionData stunning("stun-passive");
	stunning.status("dazed", statusFile(R"(["debuff", "stun"])"));
	stunning.featBoard(
		"training",
		std::string(R"({
			"version": 1, "displayNameKey": "feat-board.fixture.name", "rootNode": "root",
			"nodes": [
				{"id": "root", "displayNameKey": "feat.fixture.name", "descriptionKey": "feat.fixture.description",
				 "position": [0, 0], "kind": "root", "neighbours": ["earned"], "minimumCompletedNodes": 0,
				 "requirements": [], "rewards": []},
				{"id": "earned", "displayNameKey": "feat.fixture.name", "descriptionKey": "feat.fixture.description",
				 "position": [1, 0], "kind": "passive", "neighbours": ["root"], "minimumCompletedNodes": 0,
				 "requirements": [)") +
			requirement("hit", "[]") +
			R"(], "rewards": [{"id": "always-dazed", "type": "unlockPassive", "status": "dazed"}]}]})");

	pg::Registries registries;
	try
	{
		registries.loadAll(stunning.path());
		FAIL() << "a passive is an infinite status, and a permanently stunned creature never plays";
	} catch (const pg::JsonError &error)
	{
		EXPECT_NE(error.message().find("stun"), std::string::npos);
	}

	// The rule is the stun tag's, not every status's.
	const ProgressionData ordinary("ordinary-passive");
	ordinary.status("thorny", statusFile(R"(["buff"])"));
	ordinary.featBoard(
		"training",
		std::string(R"({
			"version": 1, "displayNameKey": "feat-board.fixture.name", "rootNode": "root",
			"nodes": [
				{"id": "root", "displayNameKey": "feat.fixture.name", "descriptionKey": "feat.fixture.description",
				 "position": [0, 0], "kind": "root", "neighbours": ["earned"], "minimumCompletedNodes": 0,
				 "requirements": [], "rewards": []},
				{"id": "earned", "displayNameKey": "feat.fixture.name", "descriptionKey": "feat.fixture.description",
				 "position": [1, 0], "kind": "passive", "neighbours": ["root"], "minimumCompletedNodes": 0,
				 "requirements": [)") +
			requirement("hit", "[]") +
			R"(], "rewards": [{"id": "thorns", "type": "unlockPassive", "status": "thorny"}]}]})");
	EXPECT_NO_THROW(registries.loadAll(ordinary.path()));
}

TEST(ProgressionRegistriesTest, ABadBoardAbortsTheWholeLoadTransaction)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	const std::vector<std::string> boardIds = registries.featBoards().ids();
	const std::vector<std::string> abilityIds = registries.abilities().ids();
	const std::size_t statusCount = registries.statuses().size();

	// The combat definitions parse perfectly; only the board is broken. Nothing may publish.
	const ProgressionData broken("broken-board");
	broken.featBoard("training", boardFile(requirement("hit", R"(["ghost-ability"])"), UnlockGuard));
	EXPECT_THROW(registries.loadAll(broken.path()), std::exception);

	EXPECT_EQ(registries.featBoards().ids(), boardIds);
	EXPECT_EQ(registries.abilities().ids(), abilityIds);
	EXPECT_EQ(registries.statuses().size(), statusCount);
	EXPECT_TRUE(registries.featBoards().contains("training-board"));

	const ProgressionData unparsable("unparsable-board");
	unparsable.featBoard("zz-broken", "{ this is not json");
	EXPECT_THROW(registries.loadAll(unparsable.path()), std::exception);
	EXPECT_EQ(registries.featBoards().ids(), boardIds);
}

TEST(ProgressionRegistriesTest, LoadsDeterministicallyWhateverTheFileOrder)
{
	const ProgressionData data("determinism");

	// Authored in reverse lexical order: neither directory iteration nor creation order may leak
	// into the registry or into a board's node vector.
	for (const std::string_view id : {"zulu", "mike", "delta", "alpha"})
	{
		data.featBoard(id, boardFile(requirement("hit", "[]"), UnlockGuard));
	}

	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	EXPECT_EQ(registries.featBoards().ids(), (std::vector<std::string>{"alpha", "delta", "mike", "zulu"}));

	pg::Registries reloaded;
	ASSERT_NO_THROW(reloaded.loadAll(data.path()));
	EXPECT_EQ(reloaded.featBoards().ids(), registries.featBoards().ids());

	for (const std::string &id : reloaded.featBoards().ids())
	{
		const pg::FeatBoardDefinition &board = reloaded.featBoards().get(id);
		ASSERT_EQ(board.nodes.size(), 2U);
		EXPECT_EQ(board.nodes[0].id, "root");
		EXPECT_EQ(board.nodes[1].id, "earned");
	}
}

TEST(ProgressionRegistriesTest, RejectsAFilenameThatIsNotAContentId)
{
	const ProgressionData data("bad-filename");
	data.featBoard("Training_Board", boardFile(requirement("hit", "[]"), UnlockGuard));

	pg::Registries registries;
	EXPECT_THROW(registries.loadAll(data.path()), pg::JsonError) << "the id is the filename stem, so it must be one";
}

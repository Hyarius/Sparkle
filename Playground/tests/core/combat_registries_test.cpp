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
	// A scratch copy of the shipped data whose three combat directories start out empty, so a
	// case authors exactly the graph it is about without touching the checkout.
	class CombatData
	{
	private:
		std::filesystem::path _path;

		void _clear(const std::filesystem::path &p_relative) const
		{
			const std::filesystem::path directory = _path / p_relative;
			for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(directory))
			{
				std::filesystem::remove_all(entry.path());
			}
		}

		void _write(const std::filesystem::path &p_relative, std::string_view p_content) const
		{
			const std::filesystem::path target = _path / p_relative;
			std::filesystem::create_directories(target.parent_path());
			std::ofstream stream(target, std::ios::binary | std::ios::trunc);
			stream << p_content;
		}

	public:
		explicit CombatData(std::string_view p_name) :
			_path(std::filesystem::temp_directory_path() / "sparkle-playground-combat" / p_name)
		{
			std::filesystem::remove_all(_path);
			std::filesystem::create_directories(_path);
			std::filesystem::copy(
				pg::resourceRoot() / "data",
				_path,
				std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

			// Empty combat directories are structurally allowed; the seeds only have to make
			// the shipped tree non-empty.
			_clear("statuses");
			_clear("abilities");
			_clear("battle-objects");
		}

		~CombatData()
		{
			std::error_code error;
			std::filesystem::remove_all(_path, error);
		}

		CombatData(const CombatData &) = delete;
		CombatData &operator=(const CombatData &) = delete;

		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _path;
		}

		void status(std::string_view p_id, std::string_view p_content) const
		{
			_write(std::filesystem::path("statuses") / (std::string(p_id) + ".json"), p_content);
		}

		void ability(std::string_view p_id, std::string_view p_content) const
		{
			_write(std::filesystem::path("abilities") / (std::string(p_id) + ".json"), p_content);
		}

		void battleObject(std::string_view p_id, std::string_view p_content) const
		{
			_write(std::filesystem::path("battle-objects") / (std::string(p_id) + ".json"), p_content);
		}
	};

	[[nodiscard]] std::string statusFile(
		std::string_view p_tags = "[]",
		std::string_view p_hooks = "[]",
		std::string_view p_modifiers = R"([{"id": "armor-bonus", "stat": "armor", "operation": "add",
			"value": 1, "perStack": false}])")
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "status.fixture.name",
			"descriptionKey": "fixture.description",
			"icon": [0, 0],
			"tags": )") +
			   std::string(p_tags) +
			   R"(, "stacking": {"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"},
			   "modifiers": )" +
			   std::string(p_modifiers) + R"(, "hooks": )" + std::string(p_hooks) + "}";
	}

	// A single-effect ability whose effect body the case supplies.
	[[nodiscard]] std::string abilityFile(std::string_view p_effect)
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "ability.fixture.name",
			"descriptionKey": "fixture.description",
			"icon": [0, 0],
			"cost": {"actionPoints": 1, "movementPoints": 0},
			"range": {"shape": "diamond", "minimum": 1, "maximum": 3, "requiresLineOfSight": true},
			"area": {"shape": "single", "radius": 0},
			"anchorFilter": {"allowSelf": true, "allowAllies": true, "allowEnemies": true,
			                 "allowDefeated": false, "allowEmptyCell": true},
			"affectedFilter": {"allowSelf": true, "allowAllies": true, "allowEnemies": true,
			                   "allowDefeated": false, "allowEmptyCell": true},
			"effects": [)") +
			   std::string(p_effect) + "]}";
	}

	[[nodiscard]] std::string battleObjectFile(std::string_view p_effect)
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "battle-object.fixture.name",
			"descriptionKey": "fixture.description",
			"icon": [0, 0],
			"tags": ["trap"],
			"blocksMovement": false,
			"blocksLineOfSight": false,
			"triggers": [{
				"id": "on-enter",
				"when": "unitEnteredCell",
				"targetFilter": {"allowSelf": false, "allowAllies": false, "allowEnemies": true,
				                 "allowDefeated": false, "allowEmptyCell": false},
				"maxTriggers": 1,
				"removeWhenExhausted": true,
				"effects": [)") +
			   std::string(p_effect) + "]}]}";
	}

	[[nodiscard]] std::string appliesStatus(
		std::string_view p_effectId,
		std::string_view p_statusId,
		std::string_view p_duration)
	{
		return std::string(R"({"id": ")") + std::string(p_effectId) +
			   R"(", "type": "applyStatus", "applyTo": "affectedUnits", "requiresLivingSource": true,
			   "status": ")" +
			   std::string(p_statusId) + R"(", "stacks": 1, "duration": )" + std::string(p_duration) + "}";
	}

	[[nodiscard]] std::string placesObject(std::string_view p_effectId, std::string_view p_objectId)
	{
		return std::string(R"({"id": ")") + std::string(p_effectId) +
			   R"(", "type": "placeObject", "applyTo": "anchorCell", "requiresLivingSource": true, "object": ")" +
			   std::string(p_objectId) + R"(", "duration": {"type": "ownerActivations", "count": 2}})";
	}

	[[nodiscard]] std::string removesStatus(std::string_view p_effectId, std::string_view p_statusId)
	{
		return std::string(R"({"id": ")") + std::string(p_effectId) +
			   R"(", "type": "removeStatus", "applyTo": "primaryUnit", "requiresLivingSource": false, "status": ")" +
			   std::string(p_statusId) + R"(", "stacks": 1})";
	}

	[[nodiscard]] std::string hook(std::string_view p_hookId, std::string_view p_when, std::string_view p_effect)
	{
		return std::string(R"([{"id": ")") + std::string(p_hookId) + R"(", "when": ")" + std::string(p_when) +
			   R"(", "effects": [)" + std::string(p_effect) + "]}]";
	}
}

TEST(CombatRegistriesTest, LoadsTheShippedSeedContent)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	EXPECT_EQ(registries.statuses().size(), 1U);
	EXPECT_EQ(registries.abilities().size(), 3U);
	EXPECT_EQ(registries.battleObjects().size(), 1U);

	EXPECT_EQ(
		registries.abilities().ids(),
		(std::vector<std::string>{"training-guard", "training-snare", "training-strike"}));
	ASSERT_TRUE(registries.statuses().contains("training-guarded"));
	ASSERT_TRUE(registries.battleObjects().contains("training-snare-object"));

	// The id is the filename stem, assigned by the loader.
	EXPECT_EQ(registries.statuses().get("training-guarded").id, "training-guarded");
	EXPECT_EQ(registries.abilities().get("training-strike").id, "training-strike");
	EXPECT_EQ(registries.battleObjects().get("training-snare-object").id, "training-snare-object");
}

TEST(CombatRegistriesTest, ResolvesEveryKindOfCrossReference)
{
	const CombatData data("references");
	data.status("guarded", statusFile());
	data.status(
		"cascading",
		statusFile("[]", hook("spread", "applied", appliesStatus("chain", "guarded", R"({"type": "infinite"})"))));
	data.battleObject("snare", battleObjectFile(removesStatus("free", "guarded")));
	data.ability("strike", abilityFile(appliesStatus("apply", "cascading", R"({"type": "infinite"})")));
	data.ability("trap", abilityFile(placesObject("place", "snare")));

	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()))
		<< "ability -> status, ability -> object and status -> status all resolve";

	EXPECT_EQ(registries.statuses().size(), 2U);
	EXPECT_EQ(registries.abilities().size(), 2U);
	EXPECT_EQ(registries.battleObjects().size(), 1U);
}

TEST(CombatRegistriesTest, AcceptsAStatusObjectStatusCycleWithoutRecursingAtLoad)
{
	// status A hook -> apply status B
	// status B hook -> place object C
	// object C trigger -> remove status A
	const CombatData data("cycle");
	data.status(
		"status-a",
		statusFile("[]", hook("spread", "applied", appliesStatus("apply-b", "status-b", R"({"type": "infinite"})"))));
	data.status("status-b", statusFile("[]", hook("place", "activationStart", placesObject("place-c", "object-c"))));
	data.battleObject("object-c", battleObjectFile(removesStatus("remove-a", "status-a")));

	// A status that applies itself is the degenerate cycle and is equally legal.
	data.status(
		"self-referential",
		statusFile(
			"[]",
			hook("again", "removed", appliesStatus("reapply", "self-referential", R"({"type": "infinite"})"))));

	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()))
		<< "references stay string ids, so a cycle needs no load order and no expansion";

	EXPECT_EQ(registries.statuses().size(), 3U);
	EXPECT_EQ(registries.battleObjects().size(), 1U);
}

TEST(CombatRegistriesTest, AMissingReferenceFailsAtTheOwningEffectPath)
{
	const CombatData missingStatus("missing-status");
	missingStatus.ability("strike", abilityFile(appliesStatus("apply", "ghost", R"({"type": "infinite"})")));

	pg::Registries registries;
	try
	{
		registries.loadAll(missingStatus.path());
		FAIL() << "an ability that applies an unknown status must fail the load";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.file().filename(), std::filesystem::path("strike.json"));
		EXPECT_EQ(error.path(), "$.effects[0]");
		EXPECT_NE(error.message().find("strike"), std::string::npos) << "names the owning definition";
		EXPECT_NE(error.message().find("apply"), std::string::npos) << "names the offending effect";
		EXPECT_NE(error.message().find("ghost"), std::string::npos) << "names the unresolved id";
	}

	const CombatData missingObject("missing-object");
	missingObject.ability("trap", abilityFile(placesObject("place", "ghost-object")));
	EXPECT_THROW(registries.loadAll(missingObject.path()), pg::JsonError);

	// A status referenced from the wrong registry does not accidentally resolve.
	const CombatData wrongRegistry("wrong-registry");
	wrongRegistry.status("guarded", statusFile());
	wrongRegistry.ability("trap", abilityFile(placesObject("place", "guarded")));
	EXPECT_THROW(registries.loadAll(wrongRegistry.path()), pg::JsonError)
		<< "'guarded' is a status, so it is not a battle object";
}

TEST(CombatRegistriesTest, AStunMayOnlyBeAppliedForAFiniteTimelineDuration)
{
	constexpr std::string_view stunTags = R"(["debuff", "stun"])";

	const CombatData activations("stun-activations");
	activations.status("stunned", statusFile(stunTags));
	activations.ability(
		"bash",
		abilityFile(appliesStatus("stun", "stunned", R"({"type": "ownerActivations", "count": 2})")));

	pg::Registries registries;
	try
	{
		registries.loadAll(activations.path());
		FAIL() << "a stunned unit cannot consume its own turn to expire the stun";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.path(), "$.effects[0]");
		EXPECT_NE(error.message().find("timeline"), std::string::npos);
	}

	const CombatData infinite("stun-infinite");
	infinite.status("stunned", statusFile(stunTags));
	infinite.ability("bash", abilityFile(appliesStatus("stun", "stunned", R"({"type": "infinite"})")));
	EXPECT_THROW(registries.loadAll(infinite.path()), pg::JsonError) << "an infinite stun is never playable";

	const CombatData timeline("stun-timeline");
	timeline.status("stunned", statusFile(stunTags));
	timeline.ability("bash", abilityFile(appliesStatus("stun", "stunned", R"({"type": "timeline", "seconds": 2.0})")));
	EXPECT_NO_THROW(registries.loadAll(timeline.path()));

	// The restriction is the stun tag's, not every status's: an ordinary status takes any of
	// the three durations.
	const CombatData ordinary("ordinary-durations");
	ordinary.status("guarded", statusFile(R"(["buff"])"));
	ordinary.ability("a", abilityFile(appliesStatus("apply", "guarded", R"({"type": "timeline", "seconds": 2.0})")));
	ordinary.ability(
		"b",
		abilityFile(appliesStatus("apply", "guarded", R"({"type": "ownerActivations", "count": 2})")));
	ordinary.ability("c", abilityFile(appliesStatus("apply", "guarded", R"({"type": "infinite"})")));
	EXPECT_NO_THROW(registries.loadAll(ordinary.path()));
	EXPECT_EQ(registries.abilities().size(), 3U);
}

TEST(CombatRegistriesTest, AFailedReloadLeavesThePreviouslyPublishedRegistriesUntouched)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));
	const std::vector<std::string> abilityIds = registries.abilities().ids();

	// A parse failure in one domain and a dangling reference in another: neither may publish
	// the definitions that did parse.
	const CombatData broken("broken-reload");
	broken.status("guarded", statusFile());
	broken.ability("strike", abilityFile(appliesStatus("apply", "ghost", R"({"type": "infinite"})")));
	EXPECT_THROW(registries.loadAll(broken.path()), std::exception);

	EXPECT_EQ(registries.statuses().size(), 1U);
	EXPECT_EQ(registries.abilities().size(), 3U);
	EXPECT_EQ(registries.battleObjects().size(), 1U);
	EXPECT_EQ(registries.abilities().ids(), abilityIds);
	EXPECT_TRUE(registries.statuses().contains("training-guarded"));

	const CombatData unparsable("unparsable-reload");
	unparsable.ability("zz-broken", "{ this is not json");
	EXPECT_THROW(registries.loadAll(unparsable.path()), std::exception);
	EXPECT_EQ(registries.abilities().ids(), abilityIds);
}

TEST(CombatRegistriesTest, LoadsDeterministicallyWhateverTheFilesystemOrder)
{
	const CombatData data("determinism");

	// Authored in reverse lexical order: directory iteration order must not leak into the
	// registry.
	for (const std::string_view id : {"zulu", "mike", "delta", "alpha"})
	{
		data.status(id, statusFile());
	}

	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));

	EXPECT_EQ(
		registries.statuses().ids(),
		(std::vector<std::string>{"alpha", "delta", "mike", "zulu"}));

	pg::Registries reloaded;
	ASSERT_NO_THROW(reloaded.loadAll(data.path()));
	EXPECT_EQ(reloaded.statuses().ids(), registries.statuses().ids());
}

TEST(CombatRegistriesTest, RejectsAFilenameThatIsNotAContentId)
{
	const CombatData data("bad-filename");
	data.status("Guarded", statusFile());

	pg::Registries registries;
	EXPECT_THROW(registries.loadAll(data.path()), pg::JsonError) << "the id is the filename stem, so it must be one";
}

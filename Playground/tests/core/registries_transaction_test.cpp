#include <gtest/gtest.h>

#include "core/paths.hpp"
#include "core/registries.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace
{
	// A scratch copy of the shipped data, so a case can break exactly one file without
	// touching the checkout.
	class DataDirectoryCopy
	{
	private:
		std::filesystem::path _path;

	public:
		explicit DataDirectoryCopy(std::string_view p_name) :
			_path(std::filesystem::temp_directory_path() / "sparkle-playground-tests" / p_name)
		{
			std::filesystem::remove_all(_path);
			std::filesystem::create_directories(_path);
			std::filesystem::copy(
				pg::resourceRoot() / "data",
				_path,
				std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
		}

		~DataDirectoryCopy()
		{
			std::error_code error;
			std::filesystem::remove_all(_path, error);
		}

		DataDirectoryCopy(const DataDirectoryCopy &) = delete;
		DataDirectoryCopy &operator=(const DataDirectoryCopy &) = delete;

		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _path;
		}

		void write(const std::filesystem::path &p_relative, std::string_view p_content) const
		{
			const std::filesystem::path target = _path / p_relative;
			std::filesystem::create_directories(target.parent_path());
			std::ofstream stream(target, std::ios::binary | std::ios::trunc);
			stream << p_content;
		}
	};

	void expectShippedRules(const pg::Registries &p_registries)
	{
		EXPECT_DOUBLE_EQ(p_registries.gameRules().maxVerticalTraversalGap, 0.5);
		EXPECT_EQ(p_registries.gameRules().battle.mitigationScale, 100);
		EXPECT_EQ(p_registries.gameRules().battle.deploymentDepth, 2);
		EXPECT_EQ(p_registries.gameRules().overlayMasks.size(), 8U);
	}
}

TEST(RegistriesTransactionTest, LoadsTheShippedDataDirectory)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	expectShippedRules(registries);
	EXPECT_GT(registries.biomes().size(), 0U);
	EXPECT_GT(registries.prefabs().size(), 0U);
}

TEST(RegistriesTransactionTest, AnInvalidRulesFileLeavesAPreviousLoadUntouched)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	const DataDirectoryCopy scratch("invalid-rules");
	scratch.write(
		"config/game-rules.json",
		R"({"version": 1, "maxVerticalTraversalGap": 0.5, "overlayMasks": {"hovered": [2, 1], "invalid": [3, 1]}})");

	EXPECT_THROW(registries.loadAll(scratch.path()), std::exception);
	expectShippedRules(registries);
	EXPECT_GT(registries.biomes().size(), 0U);
}

TEST(RegistriesTransactionTest, AFailureAfterTheRulesParseStillPublishesNothing)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	// Rules that are valid on their own - so they would be published by a parser that wrote
	// straight into the registry - next to a definition file that cannot load.
	const DataDirectoryCopy scratch("late-failure");
	scratch.write("config/game-rules.json", R"({
		"version": 2,
		"maxVerticalTraversalGap": 0.75,
		"battle": {
			"teamCapacity": 6,
			"abilitySlotCapacity": 8,
			"defaultBoardSize": [13, 13],
			"deploymentDepth": 3,
			"minimumStaminaSeconds": 0.2,
			"mitigationScale": 200,
			"maxCommandsPerActivation": 32,
			"maxAiCommandsPerActivation": 16,
			"maxEffectChainDepth": 16,
			"maxConditionDepth": 4
		},
		"overlayMasks": {
			"deployment": [0, 0],
			"movement": [1, 0],
			"path": [2, 0],
			"abilityRange": [3, 0],
			"areaOfEffect": [0, 1],
			"losBlocked": [1, 1],
			"hovered": [2, 1],
			"invalid": [3, 1]
		}
	})");
	scratch.write("biomes/zz-broken.json", "{ this is not json");

	EXPECT_THROW(registries.loadAll(scratch.path()), std::exception);

	// The failed reload published none of its rules, not even the ones that parsed.
	expectShippedRules(registries);
	EXPECT_EQ(registries.gameRules().battle.defaultBoardSize[0], 11);
	EXPECT_EQ(registries.gameRules().battle.minimumStamina.ticks(), 100);
	EXPECT_GT(registries.biomes().size(), 0U);
	EXPECT_GT(registries.voxels().typeCount(), 0U);
}

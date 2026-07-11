#include <gtest/gtest.h>

#include "structures/container/spk_definition_registry.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
	struct Definition
	{
		std::string id;
		int value = 0;
	};

	class TemporaryDirectory
	{
	private:
		std::filesystem::path _path;

	public:
		TemporaryDirectory()
		{
			_path = std::filesystem::temp_directory_path() / "sparkle-definition-registry-test";
			std::filesystem::remove_all(_path);
			std::filesystem::create_directories(_path);
		}

		~TemporaryDirectory()
		{
			std::error_code ignored;
			std::filesystem::remove_all(_path, ignored);
		}

		const std::filesystem::path &path() const noexcept { return _path; }

		void write(std::string_view p_name, std::string_view p_json) const
		{
			std::ofstream stream(_path / p_name, std::ios::binary);
			stream << p_json;
		}
	};

	Definition parseDefinition(std::string_view p_id, spk::JSON::Reader &p_reader)
	{
		return {.id = std::string(p_id), .value = p_reader.require<int>("value")};
	}
}

TEST(DefinitionRegistry, StoresDefinitionsAndRejectsDuplicateIds)
{
	spk::DefinitionRegistry<Definition> registry;
	registry.add("first", {.id = "first", .value = 1});

	EXPECT_TRUE(registry.contains("first"));
	EXPECT_EQ(registry.get("first").value, 1);
	EXPECT_EQ(registry.tryGet("missing"), nullptr);
	EXPECT_THROW(registry.add("first", {}), std::runtime_error);
	EXPECT_THROW((void)registry.get("missing"), std::out_of_range);

	spk::DefinitionRegistry<Definition> duplicate;
	duplicate.add("first", {.id = "first", .value = 2});
	EXPECT_THROW(registry.merge(std::move(duplicate)), std::runtime_error);
	EXPECT_EQ(registry.get("first").value, 1);
}

TEST(DefinitionRegistry, LoadsJsonFilesInStableFilenameOrderAndPassesIdsExplicitly)
{
	TemporaryDirectory directory;
	directory.write("b.json", R"({"value":2})");
	directory.write("a.json", R"({"value":1})");
	directory.write("ignored.txt", R"({"value":3})");

	spk::DefinitionRegistry<Definition> registry;
	std::vector<std::string> parsedIds;
	spk::loadJsonDirectory(registry, directory.path(), [&parsedIds](std::string_view p_id, spk::JSON::Reader &p_reader) {
		parsedIds.emplace_back(p_id);
		return parseDefinition(p_id, p_reader);
	});

	EXPECT_EQ(parsedIds, (std::vector<std::string>{"a", "b"}));
	EXPECT_EQ(registry.ids(), (std::vector<std::string>{"a", "b"}));
	EXPECT_EQ(registry.get("a").id, "a");
	EXPECT_EQ(registry.get("b").value, 2);
}

TEST(DefinitionRegistry, DirectoryLoadingIsTransactional)
{
	TemporaryDirectory directory;
	directory.write("valid.json", R"({"value":1})");
	directory.write("invalid.json", R"({"value":"wrong"})");

	spk::DefinitionRegistry<Definition> registry;
	registry.add("existing", {.id = "existing", .value = 7});
	EXPECT_THROW(spk::loadJsonDirectory(registry, directory.path(), parseDefinition), spk::JSON::Error);

	EXPECT_EQ(registry.ids(), (std::vector<std::string>{"existing"}));
	EXPECT_EQ(registry.get("existing").value, 7);
}

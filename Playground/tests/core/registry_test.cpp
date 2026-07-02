#include "core/registry.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
	struct TestDefinition
	{
		std::string id;
		int value = 0;
	};

	struct TestBase
	{
		virtual ~TestBase() = default;
		int value = 0;
	};

	struct TestDerived : TestBase
	{
	};

	TestDefinition parseDefinition(pg::JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "value"});
		if (p_reader.require<int>("version") != 1)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported version");
		}
		return TestDefinition{.value = p_reader.require<int>("value")};
	}

	class TemporaryDirectory
	{
	private:
		std::filesystem::path _path;

	public:
		TemporaryDirectory()
		{
			static std::atomic<unsigned int> counter = 0;
			const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
			_path = std::filesystem::temp_directory_path() /
					("sparkle-playground-test-" + std::to_string(timestamp) + "-" + std::to_string(counter++));
			std::filesystem::create_directories(_path);
		}

		~TemporaryDirectory()
		{
			std::error_code error;
			std::filesystem::remove_all(_path, error);
		}

		[[nodiscard]] const std::filesystem::path &path() const
		{
			return _path;
		}

		void write(const std::string &p_name, int p_value) const
		{
			std::ofstream stream(_path / p_name);
			stream << "{\"version\": 1, \"value\": " << p_value << "}";
		}
	};
}

TEST(Registry, LoadsDefinitionsInSortedIdOrder)
{
	TemporaryDirectory directory;
	directory.write("second.json", 2);
	directory.write("first.json", 1);

	pg::Registry<TestDefinition> registry;
	registry.load(directory.path(), parseDefinition);

	EXPECT_EQ(registry.ids(), (std::vector<std::string>{"first", "second"}));
	EXPECT_EQ(registry.get("first").id, "first");
	EXPECT_EQ(registry.get("second").value, 2);
	EXPECT_EQ(registry.tryGet("missing"), nullptr);
}

TEST(Registry, RejectsDuplicateId)
{
	TemporaryDirectory directory;
	directory.write("same.json", 1);

	pg::Registry<TestDefinition> registry;
	registry.load(directory.path(), parseDefinition);

	try
	{
		registry.load(directory.path(), parseDefinition);
		FAIL() << "Expected pg::JsonError";
	} catch (const pg::JsonError &error)
	{
		EXPECT_NE(std::string(error.what()).find("duplicate registry id 'same'"), std::string::npos);
	}
}

TEST(Registry, RejectsMissingDirectory)
{
	TemporaryDirectory directory;
	const std::filesystem::path missing = directory.path() / "missing";
	pg::Registry<TestDefinition> registry;

	try
	{
		registry.load(missing, parseDefinition);
		FAIL() << "Expected pg::JsonError";
	} catch (const pg::JsonError &error)
	{
		EXPECT_NE(std::string(error.what()).find("directory does not exist"), std::string::npos);
	}
}

TEST(PolymorphicFactory, DispatchesRegisteredType)
{
	pg::PolymorphicFactory<TestBase> factory;
	factory.registerType("sample", [](pg::JsonReader &p_reader) -> std::unique_ptr<TestBase> {
		p_reader.forbidUnknown({"type", "value"});
		auto result = std::make_unique<TestDerived>();
		result->value = p_reader.require<int>("value");
		return result;
	});

	const nlohmann::json json = {{"type", "sample"}, {"value", 12}};
	pg::JsonReader reader(json, "sample.json");
	const std::unique_ptr<TestBase> result = factory.parse(reader);
	ASSERT_NE(result, nullptr);
	EXPECT_EQ(result->value, 12);
}

TEST(PolymorphicFactory, ReportsUnknownTypePath)
{
	pg::PolymorphicFactory<TestBase> factory;
	const nlohmann::json json = {{"type", "missing"}};
	pg::JsonReader reader(json, "sample.json");

	try
	{
		(void)factory.parse(reader);
		FAIL() << "Expected pg::JsonError";
	} catch (const pg::JsonError &error)
	{
		EXPECT_NE(std::string(error.what()).find("sample.json:$.type"), std::string::npos);
	}
}

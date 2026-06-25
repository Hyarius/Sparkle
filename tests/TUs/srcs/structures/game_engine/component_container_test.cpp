#include <gtest/gtest.h>

#include "structures/container/spk_uuid.hpp"
#include "structures/game_engine/spk_component_container.hpp"
#include "structures/game_engine/spk_entity.hpp"

namespace
{
	class BoxComponent : public spk::Component
	{
	public:
		BoxComponent() = default;
	};
}

TEST(ComponentContainerTest, AddStoresComponentUnderItsEngineBucket)
{
	spk::Entity entity;
	BoxComponent &component = entity.addComponent<BoxComponent>();
	const spk::UUID engineId = spk::UUID::generate();

	spk::ComponentContainer container;
	container.add(engineId, &component);

	ASSERT_EQ(container.components(engineId).size(), 1u);
	EXPECT_EQ(container.components(engineId).front(), &component);
}

TEST(ComponentContainerTest, AddIgnoresDuplicatesAndNull)
{
	spk::Entity entity;
	BoxComponent &component = entity.addComponent<BoxComponent>();
	const spk::UUID engineId = spk::UUID::generate();

	spk::ComponentContainer container;
	container.add(engineId, &component);
	container.add(engineId, &component);
	container.add(engineId, nullptr);

	EXPECT_EQ(container.components(engineId).size(), 1u);
}

TEST(ComponentContainerTest, BucketsAreIsolatedByEngineUuid)
{
	spk::Entity entity;
	BoxComponent &first = entity.addComponent<BoxComponent>();
	BoxComponent &second = entity.addComponent<BoxComponent>();
	const spk::UUID engineA = spk::UUID::generate();
	const spk::UUID engineB = spk::UUID::generate();

	spk::ComponentContainer container;
	container.add(engineA, &first);
	container.add(engineB, &second);

	ASSERT_EQ(container.components(engineA).size(), 1u);
	EXPECT_EQ(container.components(engineA).front(), &first);
	ASSERT_EQ(container.components(engineB).size(), 1u);
	EXPECT_EQ(container.components(engineB).front(), &second);
}

TEST(ComponentContainerTest, RemoveDropsComponentAndPrunesEmptyBucket)
{
	spk::Entity entity;
	BoxComponent &component = entity.addComponent<BoxComponent>();
	const spk::UUID engineId = spk::UUID::generate();

	spk::ComponentContainer container;
	container.add(engineId, &component);
	container.remove(engineId, &component);

	EXPECT_TRUE(container.components(engineId).empty());
	EXPECT_TRUE(container.empty());
}

TEST(ComponentContainerTest, MoveRebucketsBetweenEngines)
{
	spk::Entity entity;
	BoxComponent &component = entity.addComponent<BoxComponent>();
	const spk::UUID engineA = spk::UUID::generate();
	const spk::UUID engineB = spk::UUID::generate();

	spk::ComponentContainer container;
	container.add(engineA, &component);
	container.move(engineA, engineB, &component);

	EXPECT_TRUE(container.components(engineA).empty());
	ASSERT_EQ(container.components(engineB).size(), 1u);
	EXPECT_EQ(container.components(engineB).front(), &component);
}

TEST(ComponentContainerTest, ClearEngineEmptiesThatBucketOnly)
{
	spk::Entity entity;
	BoxComponent &first = entity.addComponent<BoxComponent>();
	BoxComponent &second = entity.addComponent<BoxComponent>();
	const spk::UUID engineA = spk::UUID::generate();
	const spk::UUID engineB = spk::UUID::generate();

	spk::ComponentContainer container;
	container.add(engineA, &first);
	container.add(engineB, &second);

	container.clearEngine(engineA);

	EXPECT_TRUE(container.components(engineA).empty());
	EXPECT_EQ(container.components(engineB).size(), 1u);
}

TEST(ComponentContainerTest, ComponentsForUnknownEngineIsEmpty)
{
	spk::ComponentContainer container;

	EXPECT_TRUE(container.empty());
	EXPECT_TRUE(container.components(spk::UUID::generate()).empty());
}

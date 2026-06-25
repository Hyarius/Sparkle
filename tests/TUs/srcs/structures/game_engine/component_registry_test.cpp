#include <gtest/gtest.h>

#include "structures/game_engine/spk_component_registry.hpp"
#include "structures/game_engine/spk_entity.hpp"

namespace
{
	class PositionComponent : public spk::Component
	{
	public:
		PositionComponent() = default;
	};

	class VelocityComponent : public spk::Component
	{
	public:
		VelocityComponent() = default;
	};
}

TEST(ComponentRegistryTest, AddBucketsComponentsByType)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();
	VelocityComponent &velocity = entity.addComponent<VelocityComponent>();

	spk::ComponentRegistry registry;
	registry.add(&position);
	registry.add(&velocity);

	EXPECT_EQ(registry.container<PositionComponent>().components().size(), 1u);
	EXPECT_EQ(registry.container<VelocityComponent>().components().size(), 1u);
}

TEST(ComponentRegistryTest, RemoveDropsFromCorrectBucket)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();

	spk::ComponentRegistry registry;
	registry.add(&position);
	registry.remove(&position);

	EXPECT_TRUE(registry.container<PositionComponent>().components().empty());
}

TEST(ComponentRegistryTest, RefreshProcessablePopulatesProcessableSubset)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();

	spk::ComponentRegistry registry;
	registry.add(&position);
	registry.refreshProcessable();

	EXPECT_EQ(registry.container<PositionComponent>().processableComponents().size(), 1u);
}

TEST(ComponentRegistryTest, RefreshIfDirtyOnlyRecomputesAfterChange)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();

	spk::ComponentRegistry registry;
	registry.add(&position);
	registry.refreshProcessableIfDirty();
	ASSERT_EQ(registry.container<PositionComponent>().processableComponents().size(), 1u);

	// Without invalidation, a stale activation change is not yet reflected.
	entity.deactivate();
	registry.refreshProcessableIfDirty();
	EXPECT_EQ(registry.container<PositionComponent>().processableComponents().size(), 1u);

	// After invalidation it is recomputed.
	registry.invalidateProcessable();
	registry.refreshProcessableIfDirty();
	EXPECT_TRUE(registry.container<PositionComponent>().processableComponents().empty());
}

TEST(ComponentRegistryTest, ClearEmptiesAllContainers)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();

	spk::ComponentRegistry registry;
	registry.add(&position);
	registry.clear();

	EXPECT_TRUE(registry.container<PositionComponent>().components().empty());
}

TEST(ComponentRegistryTest, NullAndUnknownComponentsAreIgnored)
{
	spk::ComponentRegistry registry;

	registry.add(nullptr);
	registry.remove(nullptr);

	spk::Entity entity;
	PositionComponent &component = entity.addComponent<PositionComponent>();

	registry.remove(&component);

	SUCCEED();
}

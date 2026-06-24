#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/game_engine/spk_entity.hpp"

namespace
{
	class HealthComponent : public spk::Component
	{
	public:
		int points = 0;

		HealthComponent() = default;
		explicit HealthComponent(int p_points) :
			points(p_points)
		{
		}
	};

	class ArmorComponent : public spk::Component
	{
	public:
		ArmorComponent() = default;
	};
}

TEST(EntityTest, DefaultEntityHasNonNullUuid)
{
	spk::Entity entity;

	EXPECT_FALSE(entity.uuid().isNull());
}

TEST(EntityTest, EntityCanBeConstructedWithExplicitUuid)
{
	spk::UUID uuid = spk::UUID::generate();
	spk::Entity entity(uuid);

	EXPECT_EQ(entity.uuid(), uuid);
}

TEST(EntityTest, AddComponentReturnsReferenceAndForwardsArguments)
{
	spk::Entity entity;

	HealthComponent &health = entity.addComponent<HealthComponent>(50);

	EXPECT_EQ(health.points, 50);
	EXPECT_EQ(entity.components().size(), 1u);
}

TEST(EntityTest, ComponentLookupFindsAddedComponent)
{
	spk::Entity entity;
	HealthComponent &health = entity.addComponent<HealthComponent>(10);

	EXPECT_EQ(entity.component<HealthComponent>(), &health);
	EXPECT_EQ(entity.component<ArmorComponent>(), nullptr);
}

TEST(EntityTest, RequireComponentThrowsWhenMissing)
{
	spk::Entity entity;

	EXPECT_THROW((void)entity.requireComponent<HealthComponent>(), std::runtime_error);
}

TEST(EntityTest, RemoveComponentByReferenceDropsIt)
{
	spk::Entity entity;
	HealthComponent &health = entity.addComponent<HealthComponent>();

	entity.removeComponent(health);

	EXPECT_EQ(entity.component<HealthComponent>(), nullptr);
	EXPECT_TRUE(entity.components().empty());
}

TEST(EntityTest, RemoveComponentByTypeDropsIt)
{
	spk::Entity entity;
	entity.addComponent<HealthComponent>();

	entity.removeComponent<HealthComponent>();

	EXPECT_EQ(entity.component<HealthComponent>(), nullptr);
}

TEST(EntityTest, ClearComponentsEmptiesEntity)
{
	spk::Entity entity;
	entity.addComponent<HealthComponent>();
	entity.addComponent<ArmorComponent>();

	entity.clearComponents();

	EXPECT_TRUE(entity.components().empty());
}

TEST(EntityTest, HierarchyActivationFollowsAncestors)
{
	spk::Entity parent;
	spk::Entity child(&parent);

	EXPECT_TRUE(child.isHierarchyActivated());

	parent.deactivate();

	EXPECT_TRUE(child.isActivated());
	EXPECT_FALSE(child.isHierarchyActivated());

	parent.activate();

	EXPECT_TRUE(child.isHierarchyActivated());
}

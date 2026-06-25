#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/game_engine/spk_game_engine.hpp"
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

	class MissingComponent : public spk::Component
	{
	public:
		MissingComponent() = default;
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
	const spk::Entity &constEntity = entity;

	EXPECT_EQ(entity.component<HealthComponent>(), &health);
	EXPECT_EQ(entity.component<ArmorComponent>(), nullptr);
	EXPECT_EQ(constEntity.component<HealthComponent>(), static_cast<const HealthComponent *>(&health));
	EXPECT_EQ(constEntity.component<ArmorComponent>(), nullptr);
}

TEST(EntityTest, ConstComponentLookupReturnsNullWhenEntityIsEmpty)
{
	const spk::Entity entity;

	EXPECT_EQ(entity.component<HealthComponent>(), nullptr);
}

TEST(EntityTest, RequireComponentThrowsWhenMissing)
{
	spk::Entity entity;

	EXPECT_THROW((void)entity.requireComponent<HealthComponent>(), std::runtime_error);

	const spk::Entity &constEntity = entity;
	EXPECT_THROW((void)constEntity.requireComponent<HealthComponent>(), std::runtime_error);
}

TEST(EntityTest, RequireComponentReturnsExistingComponent)
{
	spk::Entity entity;
	HealthComponent &health = entity.addComponent<HealthComponent>(15);
	const spk::Entity &constEntity = entity;

	EXPECT_EQ(&entity.requireComponent<HealthComponent>(), &health);
	EXPECT_EQ(&constEntity.requireComponent<HealthComponent>(), static_cast<const HealthComponent *>(&health));
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
	entity.removeComponent<ArmorComponent>();

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

TEST(EntityTest, ComponentManagementInsideEngineCoversRegistryPaths)
{
	spk::GameEngine engine;
	spk::Entity entity;
	engine.addEntity(&entity);

	HealthComponent &component = entity.addComponent<HealthComponent>(5);
	EXPECT_EQ(entity.components().size(), 1u);

	entity.deactivate();
	entity.activate();

	entity.removeComponent(component);
	EXPECT_TRUE(entity.components().empty());
}

TEST(EntityTest, LookupAndRemoveEdgeCases)
{
	spk::Entity entity;
	entity.addComponent<HealthComponent>(1);
	ArmorComponent &armor = entity.addComponent<ArmorComponent>();

	EXPECT_EQ(entity.component<ArmorComponent>(), &armor);

	entity.removeComponent<MissingComponent>();
	EXPECT_EQ(entity.components().size(), 2u);

	spk::Entity otherEntity;
	HealthComponent &foreign = otherEntity.addComponent<HealthComponent>(2);
	entity.removeComponent(foreign);
	EXPECT_EQ(entity.components().size(), 2u);

	EXPECT_EQ(entity.requireComponent<HealthComponent>().points, 1);
}

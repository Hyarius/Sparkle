#include <gtest/gtest.h>

#include <typeindex>
#include <typeinfo>

#include "structures/container/spk_uuid.hpp"
#include "structures/game_engine/spk_component_registry.hpp"
#include "structures/game_engine/spk_component_store.hpp"
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

TEST(ComponentStoreTest, AddBucketsByTypeAndEngineAndTheViewReadsThem)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();
	VelocityComponent &velocity = entity.addComponent<VelocityComponent>();
	const spk::UUID engineId = spk::UUID::generate();

	spk::ComponentStore &store = spk::ComponentStore::instance();
	store.add(std::type_index(typeid(PositionComponent)), engineId, &position);
	store.add(std::type_index(typeid(VelocityComponent)), engineId, &velocity);

	const spk::ComponentRegistry view(engineId);
	EXPECT_EQ(view.components<PositionComponent>().size(), 1u);
	EXPECT_EQ(view.components<VelocityComponent>().size(), 1u);

	store.clearEngine(engineId);
}

TEST(ComponentStoreTest, ComponentsAreIsolatedByEngineUuid)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();
	const spk::UUID engineA = spk::UUID::generate();
	const spk::UUID engineB = spk::UUID::generate();

	spk::ComponentStore &store = spk::ComponentStore::instance();
	store.add(std::type_index(typeid(PositionComponent)), engineA, &position);

	EXPECT_EQ(spk::ComponentRegistry(engineA).components<PositionComponent>().size(), 1u);
	EXPECT_TRUE(spk::ComponentRegistry(engineB).components<PositionComponent>().empty());

	store.clearEngine(engineA);
}

TEST(ComponentStoreTest, RemoveDropsFromTheBucket)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();
	const spk::UUID engineId = spk::UUID::generate();

	spk::ComponentStore &store = spk::ComponentStore::instance();
	const std::type_index type = std::type_index(typeid(PositionComponent));
	store.add(type, engineId, &position);
	store.remove(type, engineId, &position);

	EXPECT_TRUE(spk::ComponentRegistry(engineId).components<PositionComponent>().empty());
}

TEST(ComponentStoreTest, MoveMigratesComponentBetweenEngines)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();
	const spk::UUID engineA = spk::UUID::generate();
	const spk::UUID engineB = spk::UUID::generate();

	spk::ComponentStore &store = spk::ComponentStore::instance();
	const std::type_index type = std::type_index(typeid(PositionComponent));
	store.add(type, engineA, &position);
	store.move(type, engineA, engineB, &position);

	EXPECT_TRUE(spk::ComponentRegistry(engineA).components<PositionComponent>().empty());
	EXPECT_EQ(spk::ComponentRegistry(engineB).components<PositionComponent>().size(), 1u);

	store.clearEngine(engineB);
}

TEST(ComponentStoreTest, ClearEngineDropsEveryBucketForThatEngine)
{
	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();
	VelocityComponent &velocity = entity.addComponent<VelocityComponent>();
	const spk::UUID engineId = spk::UUID::generate();

	spk::ComponentStore &store = spk::ComponentStore::instance();
	store.add(std::type_index(typeid(PositionComponent)), engineId, &position);
	store.add(std::type_index(typeid(VelocityComponent)), engineId, &velocity);

	store.clearEngine(engineId);

	EXPECT_TRUE(spk::ComponentRegistry(engineId).components<PositionComponent>().empty());
	EXPECT_TRUE(spk::ComponentRegistry(engineId).components<VelocityComponent>().empty());
}

TEST(ComponentStoreTest, NullEngineIdAndNullComponentAreIgnored)
{
	spk::ComponentStore &store = spk::ComponentStore::instance();
	const std::type_index type = std::type_index(typeid(PositionComponent));

	store.add(type, spk::UUID::null(), nullptr);
	store.remove(type, spk::UUID::null(), nullptr);

	spk::Entity entity;
	PositionComponent &position = entity.addComponent<PositionComponent>();
	store.add(type, spk::UUID::null(), &position);

	EXPECT_TRUE(spk::ComponentRegistry(spk::UUID::null()).components<PositionComponent>().empty());
}

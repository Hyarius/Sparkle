#include <gtest/gtest.h>

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

TEST(ComponentContainerTest, AddStoresComponent)
{
	spk::Entity entity;
	BoxComponent &component = entity.addComponent<BoxComponent>();

	spk::ComponentContainer container;
	container.add(&component);

	ASSERT_EQ(container.components().size(), 1u);
	EXPECT_EQ(container.components().front(), &component);
}

TEST(ComponentContainerTest, AddIgnoresDuplicatesAndNull)
{
	spk::Entity entity;
	BoxComponent &component = entity.addComponent<BoxComponent>();

	spk::ComponentContainer container;
	container.add(&component);
	container.add(&component);
	container.add(nullptr);

	EXPECT_EQ(container.components().size(), 1u);
}

TEST(ComponentContainerTest, RemoveDropsComponent)
{
	spk::Entity entity;
	BoxComponent &first = entity.addComponent<BoxComponent>();
	BoxComponent &second = entity.addComponent<BoxComponent>();

	spk::ComponentContainer container;
	container.add(&first);
	container.add(&second);
	container.remove(&first);

	ASSERT_EQ(container.components().size(), 1u);
	EXPECT_EQ(container.components().front(), &second);
}

TEST(ComponentContainerTest, RefreshProcessableKeepsOnlyProcessableComponents)
{
	spk::Entity entity;
	BoxComponent &component = entity.addComponent<BoxComponent>();

	spk::ComponentContainer container;
	container.add(&component);

	container.refreshProcessable();
	EXPECT_EQ(container.processableComponents().size(), 1u);

	entity.deactivate();
	container.refreshProcessable();
	EXPECT_TRUE(container.processableComponents().empty());
}

TEST(ComponentContainerTest, EmptyReflectsContents)
{
	spk::Entity entity;
	BoxComponent &component = entity.addComponent<BoxComponent>();

	spk::ComponentContainer container;
	EXPECT_TRUE(container.empty());

	container.add(&component);
	EXPECT_FALSE(container.empty());
}

#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/game_engine/spk_component.hpp"
#include "structures/game_engine/spk_entity.hpp"

namespace
{
	class ProbeComponent : public spk::Component
	{
	public:
		ProbeComponent() = default;
	};

	class RejectingComponent : public spk::Component
	{
	protected:
		void _onAttached(spk::Entity &) override
		{
			throw std::invalid_argument("rejected entity");
		}
	};
}

TEST(ComponentTest, NewComponentStartsActivated)
{
	ProbeComponent component;

	EXPECT_TRUE(component.isActivated());
}

TEST(ComponentTest, DetachedComponentHasNoEntity)
{
	ProbeComponent component;

	EXPECT_FALSE(component.hasEntity());
	EXPECT_EQ(component.entity(), nullptr);
}

TEST(ComponentTest, DetachedComponentIsNotProcessable)
{
	ProbeComponent component;

	EXPECT_FALSE(component.isProcessable());
}

TEST(ComponentTest, AttachedActiveComponentIsProcessable)
{
	spk::Entity entity;
	ProbeComponent &component = entity.addComponent<ProbeComponent>();
	const ProbeComponent &constComponent = component;

	EXPECT_TRUE(component.hasEntity());
	EXPECT_EQ(component.entity(), &entity);
	EXPECT_EQ(constComponent.entity(), static_cast<const spk::Entity *>(&entity));
	EXPECT_TRUE(component.isProcessable());
}

TEST(ComponentTest, DeactivatedComponentIsNotProcessable)
{
	spk::Entity entity;
	ProbeComponent &component = entity.addComponent<ProbeComponent>();

	component.deactivate();

	EXPECT_FALSE(component.isProcessable());
}

TEST(ComponentTest, ComponentIsNotProcessableWhenEntityHierarchyDeactivated)
{
	spk::Entity entity;
	ProbeComponent &component = entity.addComponent<ProbeComponent>();

	entity.deactivate();

	EXPECT_TRUE(component.isActivated());
	EXPECT_FALSE(component.isProcessable());
}

TEST(ComponentTest, AttachmentHookCanRejectAnEntityBeforeInsertion)
{
	spk::Entity entity;

	EXPECT_THROW(entity.addComponent<RejectingComponent>(), std::invalid_argument);
	EXPECT_TRUE(entity.components().empty());
}

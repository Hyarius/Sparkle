#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/game_engine/spk_component_2d.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_entity_2d.hpp"

namespace
{
	class ProbeComponent2D : public spk::Component2D
	{
	public:
		ProbeComponent2D() = default;
		~ProbeComponent2D() override = default;
	};
}

TEST(Component2DTest, DetachedComponentHasNoEntity)
{
	ProbeComponent2D component;
	const ProbeComponent2D &constComponent = component;

	EXPECT_EQ(component.entity(), nullptr);
	EXPECT_EQ(constComponent.entity(), nullptr);
}

TEST(Component2DTest, CanBeAttachedToEntity2D)
{
	spk::Entity2D entity;
	ProbeComponent2D &component = entity.addComponent<ProbeComponent2D>();
	const ProbeComponent2D &constComponent = component;

	EXPECT_EQ(component.entity(), &entity);
	EXPECT_EQ(constComponent.entity(), static_cast<const spk::Entity2D *>(&entity));
}

TEST(Component2DTest, RejectsAttachmentToPlainEntity)
{
	spk::Entity entity;

	EXPECT_THROW(entity.addComponent<ProbeComponent2D>(), std::invalid_argument);
	EXPECT_TRUE(entity.components().empty());
}

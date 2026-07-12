#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/game_engine/spk_component_3d.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_entity_2d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"

namespace
{
	class ProbeComponent3D : public spk::Component3D
	{
	public:
		ProbeComponent3D() = default;
		~ProbeComponent3D() override = default;
	};
}

TEST(Component3DTest, DetachedComponentHasNoEntity)
{
	ProbeComponent3D component;
	const ProbeComponent3D &constComponent = component;

	EXPECT_EQ(component.entity(), nullptr);
	EXPECT_EQ(constComponent.entity(), nullptr);
}

TEST(Component3DTest, CanBeAttachedToEntity3D)
{
	spk::Entity3D entity;
	ProbeComponent3D &component = entity.addComponent<ProbeComponent3D>();
	const ProbeComponent3D &constComponent = component;

	EXPECT_EQ(component.entity(), &entity);
	EXPECT_EQ(constComponent.entity(), static_cast<const spk::Entity3D *>(&entity));
	EXPECT_EQ(&component.entity()->transform(), &entity.transform());
}

TEST(Component3DTest, RejectsAttachmentToPlainEntityBeforeInsertion)
{
	spk::Entity entity;

	EXPECT_THROW(entity.addComponent<ProbeComponent3D>(), std::invalid_argument);
	EXPECT_TRUE(entity.components().empty());
}

TEST(Component3DTest, RejectsAttachmentToEntity2DBeforeInsertion)
{
	spk::Entity2D entity;
	const std::size_t componentCountBefore = entity.components().size();

	EXPECT_THROW(entity.addComponent<ProbeComponent3D>(), std::invalid_argument);
	EXPECT_EQ(entity.components().size(), componentCountBefore);
}

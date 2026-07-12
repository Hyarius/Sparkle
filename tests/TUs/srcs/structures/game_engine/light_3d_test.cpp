#include <gtest/gtest.h>

#include <variant>

#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_light_3d.hpp"

TEST(Light3DTest, DefaultsAreUntyped)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	EXPECT_FALSE(light.hasType());
	EXPECT_FALSE(light.isDirectional());
	EXPECT_FALSE(light.isPoint());
	EXPECT_FALSE(light.isSpot());

	EXPECT_FLOAT_EQ(light.intensity(), 1.0f);
	EXPECT_FALSE(light.castsShadows());
	EXPECT_EQ(light.selectionPriority(), 0);

	EXPECT_FLOAT_EQ(light.color().r, 1.0f);
	EXPECT_FLOAT_EQ(light.color().g, 1.0f);
	EXPECT_FLOAT_EQ(light.color().b, 1.0f);
	EXPECT_FLOAT_EQ(light.color().a, 1.0f);
}

TEST(Light3DTest, DirectionalAccessorInitializesUntypedLight)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	spk::DirectionalLightSettings &settings = light.directional();

	EXPECT_TRUE(light.hasType());
	EXPECT_TRUE(light.isDirectional());
	EXPECT_FALSE(light.isPoint());
	EXPECT_FALSE(light.isSpot());

	EXPECT_EQ(&settings, &light.directional());
}

TEST(Light3DTest, PointAccessorInitializesUntypedLight)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	light.point().range = 12.0f;

	EXPECT_TRUE(light.hasType());
	EXPECT_FALSE(light.isDirectional());
	EXPECT_TRUE(light.isPoint());
	EXPECT_FALSE(light.isSpot());
	EXPECT_FLOAT_EQ(light.point().range, 12.0f);
}

TEST(Light3DTest, SpotAccessorInitializesUntypedLight)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	light.spot().range = 20.0f;
	light.spot().innerHalfAngleDegrees = 15.0f;
	light.spot().outerHalfAngleDegrees = 35.0f;

	EXPECT_TRUE(light.hasType());
	EXPECT_FALSE(light.isDirectional());
	EXPECT_FALSE(light.isPoint());
	EXPECT_TRUE(light.isSpot());

	EXPECT_FLOAT_EQ(light.spot().range, 20.0f);
	EXPECT_FLOAT_EQ(light.spot().innerHalfAngleDegrees, 15.0f);
	EXPECT_FLOAT_EQ(light.spot().outerHalfAngleDegrees, 35.0f);
}

TEST(Light3DTest, RepeatedTypedAccessorPreservesSettings)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	spk::PointLightSettings &firstAccess = light.point();
	firstAccess.range = 42.0f;

	spk::PointLightSettings &secondAccess = light.point();

	EXPECT_EQ(&firstAccess, &secondAccess);
	EXPECT_FLOAT_EQ(secondAccess.range, 42.0f);
}

TEST(Light3DTest, DifferentTypedAccessorThrowsAfterDirectionalSelection)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	(void)light.directional();

	EXPECT_THROW((void)light.point(), std::bad_variant_access);
	EXPECT_THROW((void)light.spot(), std::bad_variant_access);

	EXPECT_TRUE(light.isDirectional());
}

TEST(Light3DTest, DifferentTypedAccessorThrowsAfterPointSelection)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	light.point().range = 12.0f;

	EXPECT_THROW((void)light.directional(), std::bad_variant_access);
	EXPECT_THROW((void)light.spot(), std::bad_variant_access);

	EXPECT_TRUE(light.isPoint());
	EXPECT_FLOAT_EQ(light.point().range, 12.0f);
}

TEST(Light3DTest, DifferentTypedAccessorThrowsAfterSpotSelection)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	light.spot().range = 25.0f;

	EXPECT_THROW((void)light.directional(), std::bad_variant_access);
	EXPECT_THROW((void)light.point(), std::bad_variant_access);

	EXPECT_TRUE(light.isSpot());
	EXPECT_FLOAT_EQ(light.spot().range, 25.0f);
}

TEST(Light3DTest, ConstAccessorDoesNotInitializeUntypedLight)
{
	spk::Entity3D entity;
	const spk::Light3D &light = entity.addComponent<spk::Light3D>();

	EXPECT_THROW((void)light.directional(), std::bad_variant_access);
	EXPECT_THROW((void)light.point(), std::bad_variant_access);
	EXPECT_THROW((void)light.spot(), std::bad_variant_access);

	EXPECT_FALSE(light.hasType());
}

TEST(Light3DTest, ConstAccessorReturnsSelectedSettings)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	light.point().range = 18.0f;

	const spk::Light3D &constLight = light;

	EXPECT_FLOAT_EQ(constLight.point().range, 18.0f);
	EXPECT_THROW((void)constLight.directional(), std::bad_variant_access);
	EXPECT_THROW((void)constLight.spot(), std::bad_variant_access);
}

TEST(Light3DTest, SharedPropertiesAreEditedThroughReferences)
{
	spk::Entity3D entity;
	spk::Light3D &light = entity.addComponent<spk::Light3D>();

	light.color() = spk::Color(2.0f, -4.0f, 8.0f, 0.25f);
	light.intensity() = -3.0f;
	light.castsShadows() = true;
	light.selectionPriority() = 42;

	EXPECT_FLOAT_EQ(light.color().r, 2.0f);
	EXPECT_FLOAT_EQ(light.color().g, -4.0f);
	EXPECT_FLOAT_EQ(light.color().b, 8.0f);
	EXPECT_FLOAT_EQ(light.color().a, 0.25f);
	EXPECT_FLOAT_EQ(light.intensity(), -3.0f);
	EXPECT_TRUE(light.castsShadows());
	EXPECT_EQ(light.selectionPriority(), 42);
}
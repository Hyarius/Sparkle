#include <gtest/gtest.h>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity.hpp"

TEST(Camera3DAccessorsTest, ExposesAuthoredProjectionAndUpData)
{
	spk::Entity entity;
	spk::Camera3D &camera = entity.addComponent<spk::Camera3D>();

	camera.setPerspective(72.0f, 0.25f, 640.0f);
	camera.setAspectRatio(16.0f / 9.0f);
	camera.setUp({0.0f, 0.0f, 1.0f});

	EXPECT_FLOAT_EQ(camera.fieldOfView(), 72.0f);
	EXPECT_FLOAT_EQ(camera.nearPlane(), 0.25f);
	EXPECT_FLOAT_EQ(camera.farPlane(), 640.0f);
	EXPECT_FLOAT_EQ(camera.aspectRatio(), 16.0f / 9.0f);
	EXPECT_EQ(camera.up(), spk::Vector3(0.0f, 0.0f, 1.0f));
}

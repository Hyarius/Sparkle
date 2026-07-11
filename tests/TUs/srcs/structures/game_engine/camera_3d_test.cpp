#include <gtest/gtest.h>

#include "structures/game_engine/spk_camera_3d.hpp"

#include <limits>

TEST(Camera3D, BuildsExpectedPerspectiveMatrixAndInvalidatesIt)
{
	spk::Camera3D camera;
	camera.setPerspective(90.0f, 1.0f, 11.0f);
	camera.setAspectRatio(2.0f);
	const spk::Matrix4x4 &projection = camera.projectionMatrix();

	EXPECT_NEAR(projection[0][0], 0.5f, 0.0001f);
	EXPECT_NEAR(projection[1][1], 1.0f, 0.0001f);
	EXPECT_NEAR(projection[2][2], -1.2f, 0.0001f);
	EXPECT_NEAR(projection[2][3], -1.0f, 0.0001f);
	EXPECT_NEAR(projection[3][2], -2.2f, 0.0001f);

	camera.setViewportSize(300.0f, 100.0f);
	EXPECT_FLOAT_EQ(camera.aspectRatio(), 3.0f);
	EXPECT_NEAR(camera.projectionMatrix()[0][0], 1.0f / 3.0f, 0.0001f);
	camera.setViewportSize(0.0f, 100.0f);
	EXPECT_FLOAT_EQ(camera.aspectRatio(), 3.0f);
}

TEST(Camera3D, ViewAndViewProjectionUseLookAtAndProjectionTimesView)
{
	spk::Camera3D camera;
	camera.setPosition({0.0f, 0.0f, 5.0f});
	camera.setTarget({0.0f, 0.0f, 0.0f});
	camera.setUp({0.0f, 1.0f, 0.0f});
	const spk::Matrix4x4 view = camera.viewMatrix();
	const spk::Vector3 cameraSpaceOrigin = view * spk::Vector3::Zero;

	EXPECT_NEAR(cameraSpaceOrigin.x, 0.0f, 0.0001f);
	EXPECT_NEAR(cameraSpaceOrigin.y, 0.0f, 0.0001f);
	EXPECT_NEAR(cameraSpaceOrigin.z, -5.0f, 0.0001f);
	EXPECT_EQ(camera.viewProjectionMatrix(), camera.projectionMatrix() * view);
}

TEST(Camera3D, MainCameraSlotTracksLifetime)
{
	EXPECT_EQ(spk::Camera3D::mainCamera(), nullptr);
	{
		spk::Camera3D camera;
		camera.makeMain();
		EXPECT_EQ(spk::Camera3D::mainCamera(), &camera);
	}
	EXPECT_EQ(spk::Camera3D::mainCamera(), nullptr);
}

TEST(Camera3D, BuildsARayThroughAViewportPixel)
{
	spk::Camera3D camera;
	camera.setPosition({0.0f, 0.0f, 5.0f});
	camera.setTarget({0.0f, 0.0f, 0.0f});
	camera.setPerspective(90.0f, 0.1f, 100.0f);
	camera.setViewportSize(200.0f, 100.0f);

	const spk::Ray3D center = camera.rayFromViewport({200, 100}, {100, 50});
	EXPECT_EQ(center.origin, camera.position());
	EXPECT_NEAR(center.direction.x, 0.0f, 0.0001f);
	EXPECT_NEAR(center.direction.y, 0.0f, 0.0001f);
	EXPECT_NEAR(center.direction.z, -1.0f, 0.0001f);

	const spk::Ray3D upperLeft = camera.rayFromViewport({200, 100}, {0, 0});
	EXPECT_LT(upperLeft.direction.x, 0.0f);
	EXPECT_GT(upperLeft.direction.y, 0.0f);
	EXPECT_THROW((void)camera.rayFromViewport({0, 100}, {0, 0}), std::invalid_argument);
	EXPECT_THROW(
		(void)camera.rayFromViewport({200, 100}, {std::numeric_limits<float>::quiet_NaN(), 0}),
		std::invalid_argument);
}

#include <gtest/gtest.h>

#include "structures/game_engine/spk_camera_2d.hpp"
#include "structures/game_engine/spk_entity_2d.hpp"
#include "structures/game_engine/spk_sprite_renderer_2d.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"

namespace
{
	void expectVectorNear(const spk::Vector3 &p_actual, const spk::Vector3 &p_expected)
	{
		EXPECT_NEAR(p_actual.x, p_expected.x, 0.0001f);
		EXPECT_NEAR(p_actual.y, p_expected.y, 0.0001f);
		EXPECT_NEAR(p_actual.z, p_expected.z, 0.0001f);
	}
}

TEST(Entity2DTest, CreatesTransformAndExposesPosition)
{
	spk::Entity2D entity;
	const spk::Entity2D &constEntity = entity;

	EXPECT_EQ(&entity.transform(), entity.component<spk::Transform2D>());
	EXPECT_EQ(&constEntity.transform(), constEntity.component<spk::Transform2D>());
	EXPECT_EQ(entity.position(), spk::Vector2::Zero);
}

TEST(Transform2DTest, DefaultsToIdentityValuesAndMatrices)
{
	spk::Transform2D transform;

	EXPECT_EQ(transform.position(), spk::Vector2::Zero);
	EXPECT_FLOAT_EQ(transform.rotation(), 0.0f);
	EXPECT_EQ(transform.scale(), spk::Vector2(1.0f, 1.0f));
	EXPECT_FLOAT_EQ(transform.layer(), 0.0f);
	EXPECT_EQ(transform.modelTransform(), spk::Matrix4x4::identity());
	EXPECT_EQ(transform.inverseModelTransform(), spk::Matrix4x4::identity());
}

TEST(Transform2DTest, SettersAndMoveProduceExpectedModelTransform)
{
	spk::Entity2D entity;
	spk::Transform2D &transform = entity.transform();

	transform.setPosition({2.0f, 3.0f});
	transform.move({1.0f, -1.0f});
	transform.setRotation(90.0f);
	transform.setScale({2.0f, 4.0f});
	transform.setLayer(5.0f);

	EXPECT_EQ(transform.position(), spk::Vector2(3.0f, 2.0f));
	EXPECT_FLOAT_EQ(transform.rotation(), 90.0f);
	EXPECT_EQ(transform.scale(), spk::Vector2(2.0f, 4.0f));
	EXPECT_FLOAT_EQ(transform.layer(), 5.0f);
	expectVectorNear(
		transform.modelTransform() * spk::Vector3(0.0f, 0.0f, 0.0f),
		spk::Vector3(3.0f, 2.0f, 5.0f));
	expectVectorNear(
		transform.inverseModelTransform() * spk::Vector3(3.0f, 2.0f, 5.0f),
		spk::Vector3(0.0f, 0.0f, 0.0f));
}

TEST(Transform2DTest, UnchangedValuesDoNotNotifySubscribers)
{
	spk::Entity2D entity;
	spk::Transform2D &transform = entity.transform();
	int notifications = 0;
	auto contract = transform.subscribeToModelTransformEdition(
		[&notifications](const spk::Transform2D *) { ++notifications; });

	transform.setPosition(transform.position());
	transform.setRotation(transform.rotation());
	transform.setScale(transform.scale());
	transform.setLayer(transform.layer());

	EXPECT_TRUE(contract.isValid());
	EXPECT_EQ(notifications, 0);
}

TEST(Transform2DTest, EveryChangedPropertyNotifiesSubscribers)
{
	spk::Entity2D entity;
	spk::Transform2D &transform = entity.transform();
	int notifications = 0;
	const spk::Transform2D *lastTransform = nullptr;
	auto contract = transform.subscribeToModelTransformEdition(
		[&](const spk::Transform2D *p_transform)
		{
			++notifications;
			lastTransform = p_transform;
		});

	transform.setPosition({1.0f, 2.0f});
	transform.setRotation(10.0f);
	transform.setScale({2.0f, 3.0f});
	transform.setLayer(4.0f);

	EXPECT_EQ(notifications, 4);
	EXPECT_EQ(lastTransform, &transform);
}

TEST(Transform2DTest, ParentTransformAffectsChildAndPropagatesInvalidation)
{
	spk::Entity2D parent;
	spk::Entity2D child(&parent);
	parent.transform().setPosition({10.0f, 20.0f});
	child.transform().setPosition({2.0f, 3.0f});

	expectVectorNear(
		child.transform().modelTransform() * spk::Vector3::Zero,
		spk::Vector3(12.0f, 23.0f, 0.0f));

	int childNotifications = 0;
	auto contract = child.transform().subscribeToModelTransformEdition(
		[&childNotifications](const spk::Transform2D *) { ++childNotifications; });
	parent.transform().setPosition({30.0f, 40.0f});

	EXPECT_EQ(childNotifications, 1);
	expectVectorNear(
		child.transform().modelTransform() * spk::Vector3::Zero,
		spk::Vector3(32.0f, 43.0f, 0.0f));
}

TEST(Transform2DTest, FindsNearestTransformAcrossPlainEntityAncestor)
{
	spk::Entity2D root;
	spk::Entity middle(&root);
	spk::Entity2D leaf(&middle);
	root.transform().setPosition({7.0f, 8.0f});
	leaf.transform().setPosition({1.0f, 2.0f});

	expectVectorNear(
		leaf.transform().modelTransform() * spk::Vector3::Zero,
		spk::Vector3(8.0f, 10.0f, 0.0f));
}

TEST(Camera2DTest, DetachedCameraDefaultsAndViewMatrix)
{
	spk::Camera2D camera;

	EXPECT_EQ(camera.viewport(), spk::Rect2D());
	EXPECT_EQ(camera.pixelsPerUnit(), spk::Vector2(64.0f, 64.0f));
	EXPECT_EQ(camera.projectionMatrix(), spk::Matrix4x4::identity());
	EXPECT_EQ(camera.viewMatrix(), spk::Matrix4x4::identity());
}

TEST(Camera2DTest, ValidViewportBuildsProjectionAndPropertyChangesInvalidateCache)
{
	spk::Camera2D camera;
	camera.setViewport(spk::Rect2D(0, 0, 640, 320));
	camera.setPixelsPerUnit({64.0f, 32.0f});

	const spk::Matrix4x4 firstProjection = camera.projectionMatrix();
	EXPECT_NE(firstProjection, spk::Matrix4x4::identity());
	EXPECT_EQ(camera.viewport(), spk::Rect2D(0, 0, 640, 320));
	EXPECT_EQ(camera.pixelsPerUnit(), spk::Vector2(64.0f, 32.0f));

	camera.setPixelsPerUnit({32.0f, 32.0f});
	EXPECT_NE(camera.projectionMatrix(), firstProjection);
}

TEST(Camera2DTest, InvalidViewportOrPixelsPerUnitUsesIdentityProjection)
{
	spk::Camera2D camera;
	camera.setViewport(spk::Rect2D(0, 0, 640, 320));

	camera.setPixelsPerUnit({0.0f, 32.0f});
	EXPECT_EQ(camera.projectionMatrix(), spk::Matrix4x4::identity());
	camera.setPixelsPerUnit({32.0f, 0.0f});
	EXPECT_EQ(camera.projectionMatrix(), spk::Matrix4x4::identity());
	camera.setPixelsPerUnit({32.0f, 32.0f});
	camera.setViewport(spk::Rect2D(0, 0, 0, 320));
	EXPECT_EQ(camera.projectionMatrix(), spk::Matrix4x4::identity());
	camera.setViewport(spk::Rect2D(0, 0, 640, 0));
	EXPECT_EQ(camera.projectionMatrix(), spk::Matrix4x4::identity());
}

TEST(Camera2DTest, AttachedCameraViewIsEntityInverseTransform)
{
	spk::Entity2D entity;
	spk::Camera2D &camera = entity.addComponent<spk::Camera2D>();
	entity.transform().setPosition({10.0f, 20.0f});

	EXPECT_EQ(camera.viewMatrix(), entity.transform().inverseModelTransform());
}

TEST(Camera2DTest, MainCameraTracksSelectionAndDestruction)
{
	spk::Camera2D first;
	first.makeMain();
	EXPECT_EQ(spk::Camera2D::mainCamera(), &first);

	{
		spk::Camera2D second;
		second.makeMain();
		EXPECT_EQ(spk::Camera2D::mainCamera(), &second);
	}

	EXPECT_EQ(spk::Camera2D::mainCamera(), nullptr);
}

TEST(SpriteRenderer2DTest, DefaultsAndSettersExposeRenderState)
{
	spk::SpriteRenderer2D renderer;
	spk::SpriteSheet sheet;
	spk::TextureMesh2D mesh;
	const spk::SpriteSheet::Sprite sprite{{0.25f, 0.5f}, {0.5f, 0.25f}};

	EXPECT_EQ(renderer.spriteSheet(), nullptr);
	EXPECT_EQ(renderer.mesh(), nullptr);
	EXPECT_EQ(renderer.sprite(), spk::SpriteSheet::Sprite::whole);

	renderer.setSpriteSheet(&sheet);
	renderer.setMesh(&mesh);
	renderer.setSprite(sprite);

	EXPECT_EQ(renderer.spriteSheet(), &sheet);
	EXPECT_EQ(renderer.mesh(), &mesh);
	EXPECT_EQ(renderer.sprite(), sprite);
}

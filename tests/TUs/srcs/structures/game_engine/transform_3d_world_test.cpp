#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/math/spk_quaternion.hpp"

namespace
{
	void expectVectorNear(
		const spk::Vector3 &p_actual,
		const spk::Vector3 &p_expected,
		float p_epsilon = 0.0001f)
	{
		EXPECT_NEAR(p_actual.x, p_expected.x, p_epsilon);
		EXPECT_NEAR(p_actual.y, p_expected.y, p_epsilon);
		EXPECT_NEAR(p_actual.z, p_expected.z, p_epsilon);
	}
}

TEST(Transform3DWorldTest, IdentityForwardUsesLocalNegativeZ)
{
	spk::Entity3D entity;

	expectVectorNear(entity.transform().worldForward(), {0.0f, 0.0f, -1.0f});
}

TEST(Transform3DWorldTest, WorldForwardUsesParentAndLocalRotationButNotScale)
{
	spk::Entity3D parent;
	spk::Entity3D child(&parent);

	parent.transform().setRotation(
		spk::Quaternion::fromAxisAngle({0.0f, 1.0f, 0.0f}, 90.0f));
	parent.transform().setScale({2.0f, 3.0f, 4.0f});
	child.transform().setScale({8.0f, 0.0f, 0.5f});

	expectVectorNear(child.transform().worldForward(), {-1.0f, 0.0f, 0.0f});
}

TEST(Transform3DWorldTest, TranslationChangesPositionButNotDirection)
{
	spk::Entity3D entity;
	entity.transform().setPosition({4.0f, 5.0f, 6.0f});

	expectVectorNear(entity.transform().worldPosition(), {4.0f, 5.0f, 6.0f});
	expectVectorNear(entity.transform().worldForward(), {0.0f, 0.0f, -1.0f});
}

TEST(Transform3DWorldTest, WorldPositionUsesExistingMatrixVectorMultiplication)
{
	spk::Entity3D parent;
	spk::Entity3D child(&parent);
	parent.transform().setPosition({5.0f, 0.0f, 0.0f});
	child.transform().setPosition({0.0f, 3.0f, 0.0f});

	expectVectorNear(
		child.transform().worldPosition(),
		child.transform().modelTransform() * spk::Vector3::Zero);
	expectVectorNear(child.transform().worldPosition(), {5.0f, 3.0f, 0.0f});
}

TEST(Transform3DWorldTest, RejectsInvalidRotationWithoutMutation)
{
	spk::Entity3D entity;
	const spk::Quaternion before = entity.transform().rotation();

	EXPECT_THROW(
		entity.transform().setRotation(
			{0.0f, 0.0f, 0.0f, std::numeric_limits<float>::quiet_NaN()}),
		std::invalid_argument);
	EXPECT_FLOAT_EQ(entity.transform().rotation().x, before.x);
	EXPECT_FLOAT_EQ(entity.transform().rotation().y, before.y);
	EXPECT_FLOAT_EQ(entity.transform().rotation().z, before.z);
	EXPECT_FLOAT_EQ(entity.transform().rotation().w, before.w);

	EXPECT_THROW(
		entity.transform().setRotation({0.0f, 0.0f, 0.0f, 0.0f}),
		std::invalid_argument);
}

TEST(Transform3DWorldTest, ZeroScaleDoesNotInvalidateSemanticForward)
{
	spk::Entity3D entity;
	entity.transform().setScale({0.0f, 0.0f, 0.0f});

	expectVectorNear(entity.transform().worldForward(), {0.0f, 0.0f, -1.0f});
	EXPECT_THROW((void)entity.transform().inverseModelTransform(), std::exception);
}

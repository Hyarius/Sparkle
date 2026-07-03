#include <gtest/gtest.h>

#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_transform_3d.hpp"

namespace
{
	[[nodiscard]] spk::Vector3 worldOrigin(const spk::Transform3D &p_transform)
	{
		return p_transform.modelTransform() * spk::Vector3::Zero;
	}

	void expectVector(const spk::Vector3 &p_value, float p_x, float p_y, float p_z)
	{
		EXPECT_NEAR(p_value.x, p_x, 0.0001f);
		EXPECT_NEAR(p_value.y, p_y, 0.0001f);
		EXPECT_NEAR(p_value.z, p_z, 0.0001f);
	}
}

TEST(Transform3D, DefaultsToIdentityTransform)
{
	spk::Entity3D entity;
	EXPECT_EQ(entity.position(), spk::Vector3::Zero);
	EXPECT_EQ(entity.transform().scale(), spk::Vector3::One);
	EXPECT_EQ(entity.transform().modelTransform(), spk::Matrix4x4::identity());
}

TEST(Transform3D, ComposesTranslationRotationAndScaleWithAncestors)
{
	spk::Entity3D parent;
	spk::Entity3D child(&parent);
	parent.transform().setPosition({10.0f, 0.0f, 0.0f});
	parent.transform().setRotation(spk::Quaternion::fromAxisAngle({0.0f, 0.0f, 1.0f}, 90.0f));
	child.transform().setPosition({1.0f, 0.0f, 0.0f});
	child.transform().setScale({2.0f, 3.0f, 4.0f});

	expectVector(worldOrigin(child.transform()), 10.0f, 1.0f, 0.0f);
	expectVector(child.transform().modelTransform() * spk::Vector3{1.0f, 0.0f, 0.0f}, 10.0f, 3.0f, 0.0f);
}

TEST(Transform3D, AncestorEditsInvalidateModelAndInverseCaches)
{
	spk::Entity3D root;
	spk::Entity intermediate(&root);
	spk::Entity3D leaf(&intermediate);
	leaf.transform().setPosition({1.0f, 2.0f, 3.0f});
	(void)leaf.transform().modelTransform();
	(void)leaf.transform().inverseModelTransform();

	root.transform().setPosition({10.0f, 0.0f, 0.0f});
	expectVector(worldOrigin(leaf.transform()), 11.0f, 2.0f, 3.0f);
	expectVector(
		leaf.transform().inverseModelTransform() * spk::Vector3{11.0f, 2.0f, 3.0f},
		0.0f,
		0.0f,
		0.0f);
}

TEST(Transform3D, ReparentingAndDetachingInvalidateDescendantCaches)
{
	spk::Entity3D firstParent;
	spk::Entity3D secondParent;
	spk::Entity3D child(&firstParent);
	spk::Entity3D grandchild(&child);
	firstParent.transform().setPosition({5.0f, 0.0f, 0.0f});
	secondParent.transform().setPosition({20.0f, 0.0f, 0.0f});
	child.transform().setPosition({1.0f, 0.0f, 0.0f});
	grandchild.transform().setPosition({2.0f, 0.0f, 0.0f});

	expectVector(worldOrigin(grandchild.transform()), 8.0f, 0.0f, 0.0f);
	child.setParent(&secondParent);
	expectVector(worldOrigin(grandchild.transform()), 23.0f, 0.0f, 0.0f);
	child.setParent(nullptr);
	expectVector(worldOrigin(grandchild.transform()), 3.0f, 0.0f, 0.0f);
}

#include "components/entity3d.hpp"
#include "components/transform3d.hpp"

#include <gtest/gtest.h>

namespace
{
	// The model matrix maps local origin to the transform's world position, so transforming
	// (0,0,0) is a compact way to read the composed translation.
	[[nodiscard]] spk::Vector3 worldOrigin(const pg::Transform3D &p_transform)
	{
		return p_transform.modelTransform() * spk::Vector3(0.0f, 0.0f, 0.0f);
	}

	void expectVector(const spk::Vector3 &p_value, float p_x, float p_y, float p_z)
	{
		EXPECT_FLOAT_EQ(p_value.x, p_x);
		EXPECT_FLOAT_EQ(p_value.y, p_y);
		EXPECT_FLOAT_EQ(p_value.z, p_z);
	}
}

TEST(Transform3D, ComposesWithParentTransform)
{
	pg::Entity3D parent;
	pg::Entity3D child(&parent);
	parent.transform().setPosition({10.0f, 0.0f, 0.0f});
	child.transform().setPosition({1.0f, 0.0f, 0.0f});

	expectVector(worldOrigin(child.transform()), 11.0f, 0.0f, 0.0f);
}

TEST(Transform3D, MovingParentInvalidatesCachedChildMatrix)
{
	pg::Entity3D parent;
	pg::Entity3D child(&parent);
	child.transform().setPosition({1.0f, 0.0f, 0.0f});

	// Prime the child's cache against the parent's initial pose, then move the parent.
	expectVector(worldOrigin(child.transform()), 1.0f, 0.0f, 0.0f);
	parent.transform().setPosition({10.0f, 0.0f, 0.0f});

	// Without hierarchy invalidation the child would still report its stale (1,0,0).
	expectVector(worldOrigin(child.transform()), 11.0f, 0.0f, 0.0f);
}

TEST(Transform3D, PropagatesThroughMultipleLevels)
{
	pg::Entity3D root;
	pg::Entity3D middle(&root);
	pg::Entity3D leaf(&middle);
	middle.transform().setPosition({0.0f, 2.0f, 0.0f});
	leaf.transform().setPosition({0.0f, 0.0f, 3.0f});

	expectVector(worldOrigin(leaf.transform()), 0.0f, 2.0f, 3.0f);

	root.transform().setPosition({5.0f, 0.0f, 0.0f});
	expectVector(worldOrigin(leaf.transform()), 5.0f, 2.0f, 3.0f);
}

TEST(Transform3D, InverseTracksHierarchyChanges)
{
	pg::Entity3D parent;
	pg::Entity3D child(&parent);
	child.transform().setPosition({1.0f, 2.0f, 3.0f});

	// Prime, then move the parent; the inverse must follow the recomposed model matrix.
	(void)child.transform().inverseModelTransform();
	parent.transform().setPosition({10.0f, 0.0f, 0.0f});

	const spk::Vector3 mapped =
		child.transform().inverseModelTransform() * spk::Vector3(11.0f, 2.0f, 3.0f);
	expectVector(mapped, 0.0f, 0.0f, 0.0f);
}

TEST(Transform3D, LocalMoveStillInvalidates)
{
	pg::Entity3D entity;
	entity.transform().setPosition({1.0f, 1.0f, 1.0f});
	expectVector(worldOrigin(entity.transform()), 1.0f, 1.0f, 1.0f);

	entity.transform().setPosition({4.0f, 5.0f, 6.0f});
	expectVector(worldOrigin(entity.transform()), 4.0f, 5.0f, 6.0f);
}

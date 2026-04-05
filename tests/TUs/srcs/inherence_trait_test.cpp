#include <gtest/gtest.h>

#include <vector>

#include "spk_inherence_trait.hpp"

namespace
{
	class TestNode : public spk::InherenceTrait<TestNode>
	{
	public:
		TestNode() = default;
		~TestNode() = default;
	};
}

TEST(InherenceTraitTest, FreshNodeStartsWithoutParentOrChildren)
{
	TestNode node;

	EXPECT_EQ(node.parent(), nullptr);
	EXPECT_FALSE(node.hasParent());
	EXPECT_EQ(node.nbChildren(), 0u);
	EXPECT_TRUE(node.children().empty());
}

TEST(InherenceTraitTest, AddChildSetsParentAndAddsChildToChildrenList)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);

	EXPECT_EQ(child.parent(), &parent);
	EXPECT_TRUE(child.hasParent());
	ASSERT_EQ(parent.nbChildren(), 1u);
	EXPECT_EQ(parent.children()[0], &child);
	EXPECT_TRUE(parent.hasChild(&child));
}

TEST(InherenceTraitTest, AddChildIgnoresNullptr)
{
	TestNode parent;

	parent.addChild(nullptr);

	EXPECT_EQ(parent.parent(), nullptr);
	EXPECT_EQ(parent.nbChildren(), 0u);
	EXPECT_TRUE(parent.children().empty());
}

TEST(InherenceTraitTest, AddChildIgnoresSelf)
{
	TestNode node;

	node.addChild(&node);

	EXPECT_EQ(node.parent(), nullptr);
	EXPECT_FALSE(node.hasParent());
	EXPECT_EQ(node.nbChildren(), 0u);
	EXPECT_FALSE(node.hasChild(&node));
}

TEST(InherenceTraitTest, AddChildDoesNotDuplicateExistingChild)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);
	parent.addChild(&child);

	ASSERT_EQ(parent.nbChildren(), 1u);
	EXPECT_EQ(parent.children()[0], &child);
	EXPECT_EQ(child.parent(), &parent);
}

TEST(InherenceTraitTest, RemoveChildClearsParentAndRemovesFromChildrenList)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);
	ASSERT_EQ(parent.nbChildren(), 1u);
	ASSERT_EQ(child.parent(), &parent);

	parent.removeChild(&child);

	EXPECT_EQ(child.parent(), nullptr);
	EXPECT_FALSE(child.hasParent());
	EXPECT_EQ(parent.nbChildren(), 0u);
	EXPECT_FALSE(parent.hasChild(&child));
}

TEST(InherenceTraitTest, RemoveChildIgnoresNullptr)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);
	ASSERT_EQ(parent.nbChildren(), 1u);

	parent.removeChild(nullptr);

	EXPECT_EQ(parent.nbChildren(), 1u);
	EXPECT_EQ(child.parent(), &parent);
}

TEST(InherenceTraitTest, RemoveChildIgnoresNodeThatIsNotAChild)
{
	TestNode parent;
	TestNode childA;
	TestNode childB;

	parent.addChild(&childA);
	ASSERT_EQ(parent.nbChildren(), 1u);

	parent.removeChild(&childB);

	EXPECT_EQ(parent.nbChildren(), 1u);
	EXPECT_EQ(childA.parent(), &parent);
	EXPECT_EQ(childB.parent(), nullptr);
}

TEST(InherenceTraitTest, ReparentingRemovesChildFromPreviousParent)
{
	TestNode firstParent;
	TestNode secondParent;
	TestNode child;

	firstParent.addChild(&child);
	ASSERT_EQ(child.parent(), &firstParent);
	ASSERT_EQ(firstParent.nbChildren(), 1u);
	ASSERT_EQ(secondParent.nbChildren(), 0u);

	secondParent.addChild(&child);

	EXPECT_EQ(child.parent(), &secondParent);
	EXPECT_EQ(firstParent.nbChildren(), 0u);
	ASSERT_EQ(secondParent.nbChildren(), 1u);
	EXPECT_EQ(secondParent.children()[0], &child);
}

TEST(InherenceTraitTest, ClearChildrenRemovesAllChildrenAndClearsTheirParentPointers)
{
	TestNode parent;
	TestNode firstChild;
	TestNode secondChild;
	TestNode thirdChild;

	parent.addChild(&firstChild);
	parent.addChild(&secondChild);
	parent.addChild(&thirdChild);

	ASSERT_EQ(parent.nbChildren(), 3u);
	ASSERT_EQ(firstChild.parent(), &parent);
	ASSERT_EQ(secondChild.parent(), &parent);
	ASSERT_EQ(thirdChild.parent(), &parent);

	parent.clearChildren();

	EXPECT_EQ(parent.nbChildren(), 0u);
	EXPECT_TRUE(parent.children().empty());

	EXPECT_EQ(firstChild.parent(), nullptr);
	EXPECT_EQ(secondChild.parent(), nullptr);
	EXPECT_EQ(thirdChild.parent(), nullptr);
	EXPECT_FALSE(firstChild.hasParent());
	EXPECT_FALSE(secondChild.hasParent());
	EXPECT_FALSE(thirdChild.hasParent());
}

TEST(InherenceTraitTest, HasChildReturnsTrueOnlyForCurrentChildren)
{
	TestNode parent;
	TestNode child;
	TestNode other;

	EXPECT_FALSE(parent.hasChild(&child));
	EXPECT_FALSE(parent.hasChild(&other));

	parent.addChild(&child);

	EXPECT_TRUE(parent.hasChild(&child));
	EXPECT_FALSE(parent.hasChild(&other));
}

TEST(InherenceTraitTest, ChildrenAccessorReturnsMutableReferenceToChildrenList)
{
	TestNode parent;
	TestNode childA;
	TestNode childB;

	parent.addChild(&childA);
	parent.addChild(&childB);

	std::vector<TestNode*>& children = parent.children();

	ASSERT_EQ(children.size(), 2u);
	EXPECT_EQ(children[0], &childA);
	EXPECT_EQ(children[1], &childB);
}

TEST(InherenceTraitTest, ChildrenAccessorReturnsConstReferenceToChildrenList)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);

	const TestNode& constParent = parent;
	const std::vector<TestNode*>& children = constParent.children();

	ASSERT_EQ(children.size(), 1u);
	EXPECT_EQ(children[0], &child);
}

TEST(InherenceTraitTest, ChildDestructionRemovesItselfFromParent)
{
	TestNode parent;

	{
		TestNode child;
		parent.addChild(&child);

		ASSERT_EQ(parent.nbChildren(), 1u);
		ASSERT_EQ(child.parent(), &parent);
	}

	EXPECT_EQ(parent.nbChildren(), 0u);
	EXPECT_TRUE(parent.children().empty());
}

TEST(InherenceTraitTest, ParentDestructionClearsParentPointerOfChildren)
{
	TestNode childA;
	TestNode childB;

	{
		TestNode parent;
		parent.addChild(&childA);
		parent.addChild(&childB);

		ASSERT_EQ(childA.parent(), &parent);
		ASSERT_EQ(childB.parent(), &parent);
		ASSERT_EQ(parent.nbChildren(), 2u);
	}

	EXPECT_EQ(childA.parent(), nullptr);
	EXPECT_EQ(childB.parent(), nullptr);
	EXPECT_FALSE(childA.hasParent());
	EXPECT_FALSE(childB.hasParent());
}

TEST(InherenceTraitTest, ReparentingSeveralChildrenKeepsBothParentsConsistent)
{
	TestNode firstParent;
	TestNode secondParent;
	TestNode childA;
	TestNode childB;
	TestNode childC;

	firstParent.addChild(&childA);
	firstParent.addChild(&childB);
	firstParent.addChild(&childC);

	ASSERT_EQ(firstParent.nbChildren(), 3u);
	ASSERT_EQ(secondParent.nbChildren(), 0u);

	secondParent.addChild(&childB);

	EXPECT_EQ(childA.parent(), &firstParent);
	EXPECT_EQ(childB.parent(), &secondParent);
	EXPECT_EQ(childC.parent(), &firstParent);

	EXPECT_EQ(firstParent.nbChildren(), 2u);
	EXPECT_EQ(secondParent.nbChildren(), 1u);

	EXPECT_TRUE(firstParent.hasChild(&childA));
	EXPECT_FALSE(firstParent.hasChild(&childB));
	EXPECT_TRUE(firstParent.hasChild(&childC));

	EXPECT_FALSE(secondParent.hasChild(&childA));
	EXPECT_TRUE(secondParent.hasChild(&childB));
	EXPECT_FALSE(secondParent.hasChild(&childC));
}

TEST(InherenceTraitTest, RemoveChildAfterReparentingDoesNotAffectPreviousParent)
{
	TestNode firstParent;
	TestNode secondParent;
	TestNode child;

	firstParent.addChild(&child);
	secondParent.addChild(&child);

	ASSERT_EQ(firstParent.nbChildren(), 0u);
	ASSERT_EQ(secondParent.nbChildren(), 1u);
	ASSERT_EQ(child.parent(), &secondParent);

	firstParent.removeChild(&child);

	EXPECT_EQ(firstParent.nbChildren(), 0u);
	EXPECT_EQ(secondParent.nbChildren(), 1u);
	EXPECT_EQ(child.parent(), &secondParent);
}

TEST(InherenceTraitTest, ClearChildrenOnEmptyNodeIsSafe)
{
	TestNode parent;

	EXPECT_NO_THROW(parent.clearChildren());

	EXPECT_EQ(parent.nbChildren(), 0u);
	EXPECT_TRUE(parent.children().empty());
	EXPECT_EQ(parent.parent(), nullptr);
}

TEST(InherenceTraitTest, RemoveOnlyOneChildKeepsRemainingChildrenUntouched)
{
	TestNode parent;
	TestNode childA;
	TestNode childB;
	TestNode childC;

	parent.addChild(&childA);
	parent.addChild(&childB);
	parent.addChild(&childC);

	ASSERT_EQ(parent.nbChildren(), 3u);

	parent.removeChild(&childB);

	EXPECT_EQ(parent.nbChildren(), 2u);
	EXPECT_TRUE(parent.hasChild(&childA));
	EXPECT_FALSE(parent.hasChild(&childB));
	EXPECT_TRUE(parent.hasChild(&childC));

	EXPECT_EQ(childA.parent(), &parent);
	EXPECT_EQ(childB.parent(), nullptr);
	EXPECT_EQ(childC.parent(), &parent);
}

TEST(InherenceTraitTest, ParentPointerUpdatesCorrectlyAcrossMultipleReparentings)
{
	TestNode firstParent;
	TestNode secondParent;
	TestNode thirdParent;
	TestNode child;

	firstParent.addChild(&child);
	EXPECT_EQ(child.parent(), &firstParent);
	EXPECT_TRUE(firstParent.hasChild(&child));

	secondParent.addChild(&child);
	EXPECT_EQ(child.parent(), &secondParent);
	EXPECT_FALSE(firstParent.hasChild(&child));
	EXPECT_TRUE(secondParent.hasChild(&child));

	thirdParent.addChild(&child);
	EXPECT_EQ(child.parent(), &thirdParent);
	EXPECT_FALSE(secondParent.hasChild(&child));
	EXPECT_TRUE(thirdParent.hasChild(&child));

	EXPECT_EQ(firstParent.nbChildren(), 0u);
	EXPECT_EQ(secondParent.nbChildren(), 0u);
	EXPECT_EQ(thirdParent.nbChildren(), 1u);
	EXPECT_TRUE(thirdParent.hasChild(&child));
}

TEST(InherenceTraitTest, NbChildrenMatchesActualChildrenCount)
{
	TestNode parent;
	TestNode childA;
	TestNode childB;

	EXPECT_EQ(parent.nbChildren(), 0u);

	parent.addChild(&childA);
	EXPECT_EQ(parent.nbChildren(), 1u);

	parent.addChild(&childB);
	EXPECT_EQ(parent.nbChildren(), 2u);

	parent.removeChild(&childA);
	EXPECT_EQ(parent.nbChildren(), 1u);

	parent.clearChildren();
	EXPECT_EQ(parent.nbChildren(), 0u);
}
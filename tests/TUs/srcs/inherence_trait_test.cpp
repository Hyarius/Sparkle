#include <gtest/gtest.h>

#include <string>
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

	class CallbackTestNode : public spk::InherenceTrait<CallbackTestNode>
	{
	public:
		std::vector<std::string> events;

	protected:
		void onChildAdded(CallbackTestNode* p_child) override
		{
			events.emplace_back("child_added");
		}

		void onChildRemoved(CallbackTestNode* p_child) override
		{
			events.emplace_back("child_removed");
		}

		void onParentChanged(CallbackTestNode* p_oldParent, CallbackTestNode* p_newParent) override
		{
			if (p_oldParent == nullptr && p_newParent != nullptr)
			{
				events.emplace_back("parent_changed:null->parent");
			}
			else if (p_oldParent != nullptr && p_newParent == nullptr)
			{
				events.emplace_back("parent_changed:parent->null");
			}
			else if (p_oldParent != nullptr && p_newParent != nullptr)
			{
				events.emplace_back("parent_changed:parent->parent");
			}
			else
			{
				events.emplace_back("parent_changed:null->null");
			}
		}
	};
}

TEST(InherenceTraitTest, FreshNodeStartsWithoutParentOrChildren)
{
	TestNode node;

	EXPECT_EQ(node.parent(), nullptr);
	EXPECT_FALSE(node.hasParent());
	EXPECT_EQ(node.nbChildren(), 0u);
	EXPECT_TRUE(node.children().empty());
	EXPECT_TRUE(node.isRoot());
	EXPECT_TRUE(node.isLeaf());
}

TEST(InherenceTraitTest, AddChildSetsParentAndAddsChildToChildrenList)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);

	EXPECT_EQ(child.parent(), &parent);
	EXPECT_TRUE(child.hasParent());
	EXPECT_FALSE(child.isRoot());
	EXPECT_TRUE(child.isLeaf());

	ASSERT_EQ(parent.nbChildren(), 1u);
	EXPECT_EQ(parent.children()[0], &child);
	EXPECT_TRUE(parent.hasChild(&child));
	EXPECT_TRUE(parent.isRoot());
	EXPECT_FALSE(parent.isLeaf());
}

TEST(InherenceTraitTest, AddChildIgnoresNullptr)
{
	TestNode parent;

	parent.addChild(nullptr);

	EXPECT_EQ(parent.parent(), nullptr);
	EXPECT_EQ(parent.nbChildren(), 0u);
	EXPECT_TRUE(parent.children().empty());
	EXPECT_TRUE(parent.isRoot());
	EXPECT_TRUE(parent.isLeaf());
}

TEST(InherenceTraitTest, AddChildIgnoresSelf)
{
	TestNode node;

	node.addChild(&node);

	EXPECT_EQ(node.parent(), nullptr);
	EXPECT_FALSE(node.hasParent());
	EXPECT_EQ(node.nbChildren(), 0u);
	EXPECT_FALSE(node.hasChild(&node));
	EXPECT_TRUE(node.isRoot());
	EXPECT_TRUE(node.isLeaf());
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

	EXPECT_TRUE(parent.isLeaf());
	EXPECT_TRUE(child.isRoot());
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

	EXPECT_TRUE(parent.isLeaf());
	EXPECT_TRUE(firstChild.isRoot());
	EXPECT_TRUE(secondChild.isRoot());
	EXPECT_TRUE(thirdChild.isRoot());
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

TEST(InherenceTraitTest, ChildrenAccessorReturnsConstReferenceToChildrenList)
{
	TestNode parent;
	TestNode childA;
	TestNode childB;

	parent.addChild(&childA);
	parent.addChild(&childB);

	const TestNode& constParent = parent;
	const std::vector<TestNode*>& children = constParent.children();

	ASSERT_EQ(children.size(), 2u);
	EXPECT_EQ(children[0], &childA);
	EXPECT_EQ(children[1], &childB);
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
	EXPECT_TRUE(parent.isLeaf());
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
	EXPECT_TRUE(childA.isRoot());
	EXPECT_TRUE(childB.isRoot());
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
	EXPECT_TRUE(parent.isRoot());
	EXPECT_TRUE(parent.isLeaf());
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

TEST(InherenceTraitTest, SetParentAttachesNodeToParent)
{
	TestNode parent;
	TestNode child;

	child.setParent(&parent);

	EXPECT_EQ(child.parent(), &parent);
	EXPECT_TRUE(parent.hasChild(&child));
	EXPECT_EQ(parent.nbChildren(), 1u);
}

TEST(InherenceTraitTest, SetParentToNullDetachesNodeFromParent)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);
	ASSERT_EQ(child.parent(), &parent);
	ASSERT_EQ(parent.nbChildren(), 1u);

	child.setParent(nullptr);

	EXPECT_EQ(child.parent(), nullptr);
	EXPECT_FALSE(child.hasParent());
	EXPECT_EQ(parent.nbChildren(), 0u);
	EXPECT_FALSE(parent.hasChild(&child));
}

TEST(InherenceTraitTest, SetParentToSameParentDoesNothing)
{
	TestNode parent;
	TestNode child;

	child.setParent(&parent);
	ASSERT_EQ(child.parent(), &parent);
	ASSERT_EQ(parent.nbChildren(), 1u);

	child.setParent(&parent);

	EXPECT_EQ(child.parent(), &parent);
	EXPECT_EQ(parent.nbChildren(), 1u);
	EXPECT_TRUE(parent.hasChild(&child));
}

TEST(InherenceTraitTest, IsAncestorOfReturnsTrueForDirectAndIndirectAncestors)
{
	TestNode root;
	TestNode middle;
	TestNode leaf;

	root.addChild(&middle);
	middle.addChild(&leaf);

	EXPECT_TRUE(root.isAncestorOf(&middle));
	EXPECT_TRUE(root.isAncestorOf(&leaf));
	EXPECT_TRUE(middle.isAncestorOf(&leaf));

	EXPECT_FALSE(middle.isAncestorOf(&root));
	EXPECT_FALSE(leaf.isAncestorOf(&middle));
	EXPECT_FALSE(leaf.isAncestorOf(&root));
}

TEST(InherenceTraitTest, IsDescendantOfReturnsTrueForDirectAndIndirectParents)
{
	TestNode root;
	TestNode middle;
	TestNode leaf;

	root.addChild(&middle);
	middle.addChild(&leaf);

	EXPECT_TRUE(middle.isDescendantOf(&root));
	EXPECT_TRUE(leaf.isDescendantOf(&root));
	EXPECT_TRUE(leaf.isDescendantOf(&middle));

	EXPECT_FALSE(root.isDescendantOf(&middle));
	EXPECT_FALSE(root.isDescendantOf(&leaf));
	EXPECT_FALSE(middle.isDescendantOf(&leaf));
}

TEST(InherenceTraitTest, IsAncestorOfReturnsFalseForNullptr)
{
	TestNode node;

	EXPECT_FALSE(node.isAncestorOf(nullptr));
}

TEST(InherenceTraitTest, IsDescendantOfReturnsFalseForNullptr)
{
	TestNode node;

	EXPECT_FALSE(node.isDescendantOf(nullptr));
}

TEST(InherenceTraitTest, AddChildPreventsDirectCycle)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);
	ASSERT_EQ(child.parent(), &parent);

	child.addChild(&parent);

	EXPECT_EQ(parent.parent(), nullptr);
	EXPECT_EQ(child.parent(), &parent);
	EXPECT_EQ(parent.nbChildren(), 1u);
	EXPECT_EQ(child.nbChildren(), 0u);
}

TEST(InherenceTraitTest, AddChildPreventsIndirectCycle)
{
	TestNode root;
	TestNode middle;
	TestNode leaf;

	root.addChild(&middle);
	middle.addChild(&leaf);

	ASSERT_EQ(root.nbChildren(), 1u);
	ASSERT_EQ(middle.nbChildren(), 1u);
	ASSERT_EQ(leaf.nbChildren(), 0u);

	leaf.addChild(&root);

	EXPECT_EQ(root.parent(), nullptr);
	EXPECT_EQ(middle.parent(), &root);
	EXPECT_EQ(leaf.parent(), &middle);

	EXPECT_EQ(root.nbChildren(), 1u);
	EXPECT_EQ(middle.nbChildren(), 1u);
	EXPECT_EQ(leaf.nbChildren(), 0u);
}

TEST(InherenceTraitTest, SetParentPreventsCycle)
{
	TestNode root;
	TestNode middle;
	TestNode leaf;

	root.addChild(&middle);
	middle.addChild(&leaf);

	root.setParent(&leaf);

	EXPECT_EQ(root.parent(), nullptr);
	EXPECT_EQ(middle.parent(), &root);
	EXPECT_EQ(leaf.parent(), &middle);
}

TEST(InherenceTraitTest, RootAndLeafStateUpdatesCorrectlyAcrossHierarchyChanges)
{
	TestNode root;
	TestNode child;
	TestNode grandChild;

	EXPECT_TRUE(root.isRoot());
	EXPECT_TRUE(root.isLeaf());

	root.addChild(&child);
	EXPECT_TRUE(root.isRoot());
	EXPECT_FALSE(root.isLeaf());
	EXPECT_FALSE(child.isRoot());
	EXPECT_TRUE(child.isLeaf());

	child.addChild(&grandChild);
	EXPECT_FALSE(child.isLeaf());
	EXPECT_FALSE(grandChild.isRoot());
	EXPECT_TRUE(grandChild.isLeaf());

	child.removeChild(&grandChild);
	EXPECT_TRUE(child.isLeaf());
	EXPECT_TRUE(grandChild.isRoot());
	EXPECT_TRUE(grandChild.isLeaf());
}

TEST(InherenceTraitTest, CallbackOnChildAddedIsCalled)
{
	CallbackTestNode parent;
	CallbackTestNode child;

	parent.addChild(&child);

	ASSERT_EQ(parent.events.size(), 1u);
	EXPECT_EQ(parent.events[0], "child_added");
}

TEST(InherenceTraitTest, CallbackOnChildRemovedIsCalled)
{
	CallbackTestNode parent;
	CallbackTestNode child;

	parent.addChild(&child);
	parent.events.clear();

	parent.removeChild(&child);

	ASSERT_EQ(parent.events.size(), 1u);
	EXPECT_EQ(parent.events[0], "child_removed");
}

TEST(InherenceTraitTest, CallbackOnParentChangedIsCalledWhenAttached)
{
	CallbackTestNode parent;
	CallbackTestNode child;

	child.setParent(&parent);

	ASSERT_EQ(child.events.size(), 1u);
	EXPECT_EQ(child.events[0], "parent_changed:null->parent");
}

TEST(InherenceTraitTest, CallbackOnParentChangedIsCalledWhenDetached)
{
	CallbackTestNode parent;
	CallbackTestNode child;

	child.setParent(&parent);
	child.events.clear();

	child.setParent(nullptr);

	ASSERT_EQ(child.events.size(), 1u);
	EXPECT_EQ(child.events[0], "parent_changed:parent->null");
}

TEST(InherenceTraitTest, ReparentingTriggersRemoveAddAndParentChangedCallbacks)
{
	CallbackTestNode firstParent;
	CallbackTestNode secondParent;
	CallbackTestNode child;

	child.setParent(&firstParent);

	firstParent.events.clear();
	secondParent.events.clear();
	child.events.clear();

	child.setParent(&secondParent);

	ASSERT_EQ(firstParent.events.size(), 1u);
	EXPECT_EQ(firstParent.events[0], "child_removed");

	ASSERT_EQ(secondParent.events.size(), 1u);
	EXPECT_EQ(secondParent.events[0], "child_added");

	ASSERT_EQ(child.events.size(), 1u);
	EXPECT_EQ(child.events[0], "parent_changed:parent->parent");
}
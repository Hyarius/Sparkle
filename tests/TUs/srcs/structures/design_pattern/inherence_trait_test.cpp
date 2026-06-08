#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "structures/design_pattern/spk_inherence_trait.hpp"

namespace
{
	class TestNode : public spk::HierarchyTrait<TestNode>
	{
	public:
		TestNode() = default;
		~TestNode() = default;

		void runDuringTraversal(const std::function<void()>& p_callback)
		{
			spk::HierarchyTrait<TestNode>::HierarchyMutationGuard guard(this);
			p_callback();
		}
	};

	class CallbackTestNode : public spk::HierarchyTrait<CallbackTestNode>
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

TEST(HierarchyTraitTest, FreshNodeStartsWithoutParentOrChildren)
{
	TestNode node;

	EXPECT_EQ(node.parent(), nullptr);
	EXPECT_FALSE(node.hasParent());
	EXPECT_EQ(node.nbChildren(), 0u);
	EXPECT_TRUE(node.children().empty());
	EXPECT_TRUE(node.isRoot());
	EXPECT_TRUE(node.isLeaf());
}

TEST(HierarchyTraitTest, AddChildSetsParentAndAddsChildToChildrenList)
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

TEST(HierarchyTraitTest, AddChildIgnoresNullptr)
{
	TestNode parent;

	parent.addChild(nullptr);

	EXPECT_EQ(parent.parent(), nullptr);
	EXPECT_EQ(parent.nbChildren(), 0u);
	EXPECT_TRUE(parent.children().empty());
	EXPECT_TRUE(parent.isRoot());
	EXPECT_TRUE(parent.isLeaf());
}

TEST(HierarchyTraitTest, AddChildIgnoresSelf)
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

TEST(HierarchyTraitTest, AddChildDoesNotDuplicateExistingChild)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);
	parent.addChild(&child);

	ASSERT_EQ(parent.nbChildren(), 1u);
	EXPECT_EQ(parent.children()[0], &child);
	EXPECT_EQ(child.parent(), &parent);
}

TEST(HierarchyTraitTest, RemoveChildClearsParentAndRemovesFromChildrenList)
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

TEST(HierarchyTraitTest, RemoveChildIgnoresNullptr)
{
	TestNode parent;
	TestNode child;

	parent.addChild(&child);
	ASSERT_EQ(parent.nbChildren(), 1u);

	parent.removeChild(nullptr);

	EXPECT_EQ(parent.nbChildren(), 1u);
	EXPECT_EQ(child.parent(), &parent);
}

TEST(HierarchyTraitTest, RemoveChildIgnoresNodeThatIsNotAChild)
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

TEST(HierarchyTraitTest, ReparentingRemovesChildFromPreviousParent)
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

TEST(HierarchyTraitTest, ClearChildrenRemovesAllChildrenAndClearsTheirParentPointers)
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

TEST(HierarchyTraitTest, HasChildReturnsTrueOnlyForCurrentChildren)
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

TEST(HierarchyTraitTest, ChildrenAccessorReturnsConstReferenceToChildrenList)
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

TEST(HierarchyTraitTest, ChildDestructionRemovesItselfFromParent)
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

TEST(HierarchyTraitTest, ParentDestructionClearsParentPointerOfChildren)
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

TEST(HierarchyTraitTest, ChildDestructionNotifiesStillLiveParentOfDetachment)
{
	CallbackTestNode parent;
	auto child = std::make_unique<CallbackTestNode>();

	parent.addChild(child.get());
	parent.events.clear();
	child->events.clear();

	child.reset();

	EXPECT_TRUE(parent.children().empty());
	ASSERT_EQ(parent.events.size(), 1u);
	EXPECT_EQ(parent.events[0], "child_removed");
}

TEST(HierarchyTraitTest, ParentDestructionNotifiesStillLiveChildrenOfDetachment)
{
	CallbackTestNode child;

	{
		auto parent = std::make_unique<CallbackTestNode>();
		parent->addChild(&child);
		child.events.clear();
	}

	ASSERT_EQ(child.events.size(), 1u);
	EXPECT_EQ(child.events[0], "parent_changed:parent->null");
}

TEST(HierarchyTraitTest, ReparentingSeveralChildrenKeepsBothParentsConsistent)
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

TEST(HierarchyTraitTest, RemoveChildAfterReparentingDoesNotAffectPreviousParent)
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

TEST(HierarchyTraitTest, ClearChildrenOnEmptyNodeIsSafe)
{
	TestNode parent;

	EXPECT_NO_THROW(parent.clearChildren());

	EXPECT_EQ(parent.nbChildren(), 0u);
	EXPECT_TRUE(parent.children().empty());
	EXPECT_EQ(parent.parent(), nullptr);
	EXPECT_TRUE(parent.isRoot());
	EXPECT_TRUE(parent.isLeaf());
}

TEST(HierarchyTraitTest, RemoveOnlyOneChildKeepsRemainingChildrenUntouched)
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

TEST(HierarchyTraitTest, ParentPointerUpdatesCorrectlyAcrossMultipleReparentings)
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

TEST(HierarchyTraitTest, NbChildrenMatchesActualChildrenCount)
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

TEST(HierarchyTraitTest, SetParentAttachesNodeToParent)
{
	TestNode parent;
	TestNode child;

	child.setParent(&parent);

	EXPECT_EQ(child.parent(), &parent);
	EXPECT_TRUE(parent.hasChild(&child));
	EXPECT_EQ(parent.nbChildren(), 1u);
}

TEST(HierarchyTraitTest, SetParentToNullDetachesNodeFromParent)
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

TEST(HierarchyTraitTest, SetParentToSameParentDoesNothing)
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

TEST(HierarchyTraitTest, IsAncestorOfReturnsTrueForDirectAndIndirectAncestors)
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

TEST(HierarchyTraitTest, IsDescendantOfReturnsTrueForDirectAndIndirectParents)
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

TEST(HierarchyTraitTest, IsAncestorOfReturnsFalseForNullptr)
{
	TestNode node;

	EXPECT_FALSE(node.isAncestorOf(nullptr));
}

TEST(HierarchyTraitTest, IsDescendantOfReturnsFalseForNullptr)
{
	TestNode node;

	EXPECT_FALSE(node.isDescendantOf(nullptr));
}

TEST(HierarchyTraitTest, AddChildPreventsDirectCycle)
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

TEST(HierarchyTraitTest, AddChildPreventsIndirectCycle)
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

TEST(HierarchyTraitTest, SetParentPreventsCycle)
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

TEST(HierarchyTraitTest, RootAndLeafStateUpdatesCorrectlyAcrossHierarchyChanges)
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

TEST(HierarchyTraitTest, CallbackOnChildAddedIsCalled)
{
	CallbackTestNode parent;
	CallbackTestNode child;

	parent.addChild(&child);

	ASSERT_EQ(parent.events.size(), 1u);
	EXPECT_EQ(parent.events[0], "child_added");
}

TEST(HierarchyTraitTest, CallbackOnChildRemovedIsCalled)
{
	CallbackTestNode parent;
	CallbackTestNode child;

	parent.addChild(&child);
	parent.events.clear();

	parent.removeChild(&child);

	ASSERT_EQ(parent.events.size(), 1u);
	EXPECT_EQ(parent.events[0], "child_removed");
}

TEST(HierarchyTraitTest, CallbackOnParentChangedIsCalledWhenAttached)
{
	CallbackTestNode parent;
	CallbackTestNode child;

	child.setParent(&parent);

	ASSERT_EQ(child.events.size(), 1u);
	EXPECT_EQ(child.events[0], "parent_changed:null->parent");
}

TEST(HierarchyTraitTest, CallbackOnParentChangedIsCalledWhenDetached)
{
	CallbackTestNode parent;
	CallbackTestNode child;

	child.setParent(&parent);
	child.events.clear();

	child.setParent(nullptr);

	ASSERT_EQ(child.events.size(), 1u);
	EXPECT_EQ(child.events[0], "parent_changed:parent->null");
}

TEST(HierarchyTraitTest, ReparentingTriggersRemoveAddAndParentChangedCallbacks)
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

TEST(HierarchyTraitTest, SetParentDuringTraversalIsDeferredUntilTraversalEnds)
{
	TestNode firstParent;
	TestNode secondParent;
	TestNode child;

	firstParent.addChild(&child);

	firstParent.runDuringTraversal(
		[&]()
		{
			child.setParent(&secondParent);

			EXPECT_EQ(child.parent(), &firstParent);
			EXPECT_TRUE(firstParent.hasChild(&child));
			EXPECT_FALSE(secondParent.hasChild(&child));
		});

	EXPECT_EQ(child.parent(), &secondParent);
	EXPECT_FALSE(firstParent.hasChild(&child));
	EXPECT_TRUE(secondParent.hasChild(&child));
}

TEST(HierarchyTraitTest, ClearChildrenDuringTraversalIsDeferredUntilTraversalEnds)
{
	TestNode parent;
	TestNode firstChild;
	TestNode secondChild;

	parent.addChild(&firstChild);
	parent.addChild(&secondChild);

	parent.runDuringTraversal(
		[&]()
		{
			parent.clearChildren();

			EXPECT_EQ(parent.nbChildren(), 2u);
			EXPECT_EQ(firstChild.parent(), &parent);
			EXPECT_EQ(secondChild.parent(), &parent);
		});

	EXPECT_TRUE(parent.children().empty());
	EXPECT_EQ(firstChild.parent(), nullptr);
	EXPECT_EQ(secondChild.parent(), nullptr);
}

TEST(HierarchyTraitTest, HierarchyMutationGuardOnlyDefersMutationsTouchingGuardedTree)
{
	TestNode blockedParent;
	TestNode blockedChild;
	TestNode unrelatedParent;
	TestNode unrelatedChild;

	blockedParent.addChild(&blockedChild);

	blockedParent.runDuringTraversal(
		[&]()
		{
			blockedChild.setParent(nullptr);
			unrelatedChild.setParent(&unrelatedParent);

			EXPECT_EQ(blockedChild.parent(), &blockedParent);
			EXPECT_TRUE(blockedParent.hasChild(&blockedChild));

			EXPECT_EQ(unrelatedChild.parent(), &unrelatedParent);
			EXPECT_TRUE(unrelatedParent.hasChild(&unrelatedChild));
		});

	EXPECT_EQ(blockedChild.parent(), nullptr);
	EXPECT_FALSE(blockedParent.hasChild(&blockedChild));
	EXPECT_EQ(unrelatedChild.parent(), &unrelatedParent);
	EXPECT_TRUE(unrelatedParent.hasChild(&unrelatedChild));
}


TEST(HierarchyTraitTest, BlockHierarchyMutationsCanBeReleasedManually)
{
	TestNode firstParent;
	TestNode secondParent;
	TestNode child;

	firstParent.addChild(&child);

	spk::HierarchyTrait<TestNode>::HierarchyMutationGuard guard(&firstParent);
	child.setParent(&secondParent);

	EXPECT_EQ(child.parent(), &firstParent);
	EXPECT_TRUE(firstParent.hasChild(&child));
	EXPECT_FALSE(secondParent.hasChild(&child));

	guard.release();

	EXPECT_EQ(child.parent(), &secondParent);
	EXPECT_FALSE(firstParent.hasChild(&child));
	EXPECT_TRUE(secondParent.hasChild(&child));
}

TEST(HierarchyTraitTest, MovedHierarchyMutationGuardKeepsDeferredMutationsBlockedUntilMovedGuardIsReleased)
{
	TestNode firstParent;
	TestNode secondParent;
	TestNode child;

	firstParent.addChild(&child);

	spk::HierarchyTrait<TestNode>::HierarchyMutationGuard guard(&firstParent);
	auto movedGuard = std::move(guard);

	child.setParent(&secondParent);

	EXPECT_EQ(child.parent(), &firstParent);
	EXPECT_TRUE(firstParent.hasChild(&child));
	EXPECT_FALSE(secondParent.hasChild(&child));

	movedGuard.release();

	EXPECT_EQ(child.parent(), &secondParent);
	EXPECT_FALSE(firstParent.hasChild(&child));
	EXPECT_TRUE(secondParent.hasChild(&child));
}

TEST(HierarchyTraitTest, MoveAssigningHierarchyMutationGuardReleasesPreviousGuard)
{
	TestNode firstParent;
	TestNode secondParent;
	TestNode child;

	firstParent.addChild(&child);

	spk::HierarchyTrait<TestNode>::HierarchyMutationGuard firstGuard(&firstParent);
	spk::HierarchyTrait<TestNode>::HierarchyMutationGuard secondGuard(&secondParent);

	child.setParent(&secondParent);

	EXPECT_EQ(child.parent(), &firstParent);
	EXPECT_TRUE(firstParent.hasChild(&child));
	EXPECT_FALSE(secondParent.hasChild(&child));

	secondGuard = std::move(firstGuard);

	EXPECT_EQ(child.parent(), &firstParent);
	EXPECT_TRUE(firstParent.hasChild(&child));
	EXPECT_FALSE(secondParent.hasChild(&child));

	secondGuard.release();

	EXPECT_EQ(child.parent(), &secondParent);
	EXPECT_FALSE(firstParent.hasChild(&child));
	EXPECT_TRUE(secondParent.hasChild(&child));
}

TEST(HierarchyTraitTest, DeferredSetParentUsesLatestParentForSameNode)
{
	TestNode firstParent;
	TestNode secondParent;
	TestNode thirdParent;
	TestNode child;

	firstParent.addChild(&child);

	firstParent.runDuringTraversal(
		[&]()
		{
			child.setParent(&secondParent);
			child.setParent(&thirdParent);

			EXPECT_EQ(child.parent(), &firstParent);
			EXPECT_TRUE(firstParent.hasChild(&child));
			EXPECT_FALSE(secondParent.hasChild(&child));
			EXPECT_FALSE(thirdParent.hasChild(&child));
		});

	EXPECT_EQ(child.parent(), &thirdParent);
	EXPECT_FALSE(firstParent.hasChild(&child));
	EXPECT_FALSE(secondParent.hasChild(&child));
	EXPECT_TRUE(thirdParent.hasChild(&child));
}

TEST(HierarchyTraitTest, DeferredSetParentIsIgnoredWhenTargetNodeWasDestroyedBeforeFlush)
{
	TestNode guardedParent;
	TestNode newParent;
	auto child = std::make_unique<TestNode>();

	guardedParent.addChild(child.get());

	ASSERT_EQ(child->parent(), &guardedParent);
	ASSERT_TRUE(guardedParent.hasChild(child.get()));

	guardedParent.runDuringTraversal(
		[&]()
		{
			child->setParent(&newParent);

			EXPECT_EQ(child->parent(), &guardedParent);
			EXPECT_TRUE(guardedParent.hasChild(child.get()));
			EXPECT_FALSE(newParent.hasChild(child.get()));

			child.reset();

			EXPECT_TRUE(guardedParent.children().empty());
			EXPECT_EQ(newParent.nbChildren(), 0u);
		});

	EXPECT_TRUE(guardedParent.children().empty());
	EXPECT_EQ(newParent.nbChildren(), 0u);
}

TEST(HierarchyTraitTest, DeferredSetParentIsIgnoredWhenTargetParentWasDestroyedBeforeFlush)
{
	TestNode guardedParent;
	TestNode child;

	guardedParent.addChild(&child);

	guardedParent.runDuringTraversal(
		[&]()
		{
			auto newParent = std::make_unique<TestNode>();

			child.setParent(newParent.get());

			EXPECT_EQ(child.parent(), &guardedParent);
			EXPECT_TRUE(guardedParent.hasChild(&child));
			EXPECT_TRUE(newParent->children().empty());

			newParent.reset();
		});

	EXPECT_EQ(child.parent(), &guardedParent);
	EXPECT_TRUE(guardedParent.hasChild(&child));
}

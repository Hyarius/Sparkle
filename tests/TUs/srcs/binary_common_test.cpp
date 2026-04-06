#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "spk_binary_common.hpp"

namespace
{
	using namespace spk;
	using namespace spk::_binary_detail;
}

TEST(BinaryCommonTest, BinaryFieldKindValuesAreStable)
{
	EXPECT_EQ(static_cast<int>(spk::BinaryFieldKind::Value), 0);
	EXPECT_EQ(static_cast<int>(spk::BinaryFieldKind::Object), 1);
	EXPECT_EQ(static_cast<int>(spk::BinaryFieldKind::Array), 2);
}

TEST(BinaryCommonTest, BinaryLayoutModeValuesAreStable)
{
	EXPECT_EQ(static_cast<int>(spk::BinaryLayoutMode::Packed), 0);
	EXPECT_EQ(static_cast<int>(spk::BinaryLayoutMode::ScalarAligned), 1);
	EXPECT_EQ(static_cast<int>(spk::BinaryLayoutMode::Std140), 2);
	EXPECT_EQ(static_cast<int>(spk::BinaryLayoutMode::Std430), 3);
}

TEST(BinaryCommonTest, NodeDefaultStateIsCorrect)
{
	Node node;

	EXPECT_EQ(node.name, "");
	EXPECT_EQ(node.kind, spk::BinaryFieldKind::Value);

	EXPECT_EQ(node.offset, 0u);
	EXPECT_EQ(node.size, 0u);
	EXPECT_EQ(node.alignment, 1u);
	EXPECT_EQ(node.stride, 0u);
	EXPECT_EQ(node.count, 0u);

	EXPECT_EQ(node.valueSize, 0u);
	EXPECT_EQ(node.elementSize, 0u);

	EXPECT_EQ(node.parent, nullptr);
	EXPECT_TRUE(node.children.empty());
}

TEST(BinaryCommonTest, AlignUpReturnsSameValueWhenAlignmentIsZeroOrOne)
{
	EXPECT_EQ(alignUp(0, 0), 0u);
	EXPECT_EQ(alignUp(7, 0), 7u);
	EXPECT_EQ(alignUp(0, 1), 0u);
	EXPECT_EQ(alignUp(7, 1), 7u);
}

TEST(BinaryCommonTest, AlignUpReturnsSameValueWhenAlreadyAligned)
{
	EXPECT_EQ(alignUp(0, 4), 0u);
	EXPECT_EQ(alignUp(4, 4), 4u);
	EXPECT_EQ(alignUp(8, 4), 8u);
	EXPECT_EQ(alignUp(16, 8), 16u);
}

TEST(BinaryCommonTest, AlignUpRoundsToNextBoundaryWhenNeeded)
{
	EXPECT_EQ(alignUp(1, 4), 4u);
	EXPECT_EQ(alignUp(2, 4), 4u);
	EXPECT_EQ(alignUp(3, 4), 4u);
	EXPECT_EQ(alignUp(5, 4), 8u);
	EXPECT_EQ(alignUp(9, 8), 16u);
	EXPECT_EQ(alignUp(17, 16), 32u);
}

TEST(BinaryCommonTest, ComputeScalarAlignedAlignmentMatchesExpectedBuckets)
{
	EXPECT_EQ(computeScalarAlignedAlignment(0), 1u);
	EXPECT_EQ(computeScalarAlignedAlignment(1), 1u);
	EXPECT_EQ(computeScalarAlignedAlignment(2), 2u);
	EXPECT_EQ(computeScalarAlignedAlignment(3), 4u);
	EXPECT_EQ(computeScalarAlignedAlignment(4), 4u);
	EXPECT_EQ(computeScalarAlignedAlignment(5), 8u);
	EXPECT_EQ(computeScalarAlignedAlignment(8), 8u);
	EXPECT_EQ(computeScalarAlignedAlignment(9), 16u);
	EXPECT_EQ(computeScalarAlignedAlignment(12), 16u);
	EXPECT_EQ(computeScalarAlignedAlignment(16), 16u);
	EXPECT_EQ(computeScalarAlignedAlignment(32), 16u);
}

TEST(BinaryCommonTest, ComputeStdAlignmentFromSizeMatchesExpectedBuckets)
{
	EXPECT_EQ(computeStdAlignmentFromSize(0), 4u);
	EXPECT_EQ(computeStdAlignmentFromSize(1), 4u);
	EXPECT_EQ(computeStdAlignmentFromSize(4), 4u);
	EXPECT_EQ(computeStdAlignmentFromSize(5), 8u);
	EXPECT_EQ(computeStdAlignmentFromSize(8), 8u);
	EXPECT_EQ(computeStdAlignmentFromSize(9), 16u);
	EXPECT_EQ(computeStdAlignmentFromSize(12), 16u);
	EXPECT_EQ(computeStdAlignmentFromSize(16), 16u);
	EXPECT_EQ(computeStdAlignmentFromSize(64), 16u);
}

TEST(BinaryCommonTest, NodeCanOwnChildren)
{
	Node parent;
	parent.kind = spk::BinaryFieldKind::Object;
	parent.name = "Parent";

	std::unique_ptr<Node> childA = std::make_unique<Node>();
	childA->name = "A";
	childA->parent = &parent;

	std::unique_ptr<Node> childB = std::make_unique<Node>();
	childB->name = "B";
	childB->parent = &parent;

	parent.children.push_back(std::move(childA));
	parent.children.push_back(std::move(childB));

	ASSERT_EQ(parent.children.size(), 2u);
	ASSERT_NE(parent.children[0], nullptr);
	ASSERT_NE(parent.children[1], nullptr);

	EXPECT_EQ(parent.children[0]->name, "A");
	EXPECT_EQ(parent.children[1]->name, "B");
	EXPECT_EQ(parent.children[0]->parent, &parent);
	EXPECT_EQ(parent.children[1]->parent, &parent);
}
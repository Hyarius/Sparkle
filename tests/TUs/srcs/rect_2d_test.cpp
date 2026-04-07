#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "spk_rect_2d.hpp"

TEST(Rect2DConstructionTest, DefaultConstructorBuildsZeroRect)
{
	const spk::Rect2D rect;

	EXPECT_EQ(rect.anchor.x, 0);
	EXPECT_EQ(rect.anchor.y, 0);
	EXPECT_EQ(rect.size.x, 0u);
	EXPECT_EQ(rect.size.y, 0u);
}

TEST(Rect2DConstructionTest, AnchorAndSizeConstructorStoresValues)
{
	const spk::Rect2D rect(
		spk::Rect2D::Anchor(10, 20),
		spk::Rect2D::Size(30, 40));

	EXPECT_EQ(rect.anchor.x, 10);
	EXPECT_EQ(rect.anchor.y, 20);
	EXPECT_EQ(rect.size.x, 30u);
	EXPECT_EQ(rect.size.y, 40u);
}

TEST(Rect2DConstructionTest, AnchorWidthHeightConstructorStoresValues)
{
	const spk::Rect2D rect(
		spk::Rect2D::Anchor(10, 20),
		30,
		40);

	EXPECT_EQ(rect.anchor.x, 10);
	EXPECT_EQ(rect.anchor.y, 20);
	EXPECT_EQ(rect.size.x, 30u);
	EXPECT_EQ(rect.size.y, 40u);
}

TEST(Rect2DConstructionTest, XYAndSizeConstructorStoresValues)
{
	const spk::Rect2D rect(
		10,
		20,
		spk::Rect2D::Size(30, 40));

	EXPECT_EQ(rect.anchor.x, 10);
	EXPECT_EQ(rect.anchor.y, 20);
	EXPECT_EQ(rect.size.x, 30u);
	EXPECT_EQ(rect.size.y, 40u);
}

TEST(Rect2DConstructionTest, XYWidthHeightConstructorStoresValues)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_EQ(rect.anchor.x, 10);
	EXPECT_EQ(rect.anchor.y, 20);
	EXPECT_EQ(rect.size.x, 30u);
	EXPECT_EQ(rect.size.y, 40u);
}

TEST(Rect2DAccessorTest, AccessorsReturnExpectedValues)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_EQ(rect.x(), 10);
	EXPECT_EQ(rect.y(), 20);
	EXPECT_EQ(rect.width(), 30u);
	EXPECT_EQ(rect.height(), 40u);
}

TEST(Rect2DBoundsTest, BoundsAccessorsReturnExpectedValues)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_EQ(rect.left(), 10);
	EXPECT_EQ(rect.top(), 20);
	EXPECT_EQ(rect.right(), 40);
	EXPECT_EQ(rect.bottom(), 60);
}

TEST(Rect2DEmptyTest, EmptyReturnsTrueWhenWidthIsZero)
{
	const spk::Rect2D rect(10, 20, 0, 40);

	EXPECT_TRUE(rect.empty());
}

TEST(Rect2DEmptyTest, EmptyReturnsTrueWhenHeightIsZero)
{
	const spk::Rect2D rect(10, 20, 30, 0);

	EXPECT_TRUE(rect.empty());
}

TEST(Rect2DEmptyTest, EmptyReturnsFalseWhenWidthAndHeightAreNonZero)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_FALSE(rect.empty());
}

TEST(Rect2DAtOriginTest, AtOriginPreservesSizeAndMovesAnchorToZero)
{
	const spk::Rect2D rect(10, 20, 30, 40);
	const spk::Rect2D result = rect.atOrigin();

	EXPECT_EQ(result.anchor.x, 0);
	EXPECT_EQ(result.anchor.y, 0);
	EXPECT_EQ(result.size.x, 30u);
	EXPECT_EQ(result.size.y, 40u);
}

TEST(Rect2DContainsPointTest, ContainsReturnsTrueForPointInside)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_TRUE(rect.contains(spk::Vector2Int(15, 25)));
}

TEST(Rect2DContainsPointTest, ContainsReturnsTrueForLeftTopCorner)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_TRUE(rect.contains(spk::Vector2Int(10, 20)));
}

TEST(Rect2DContainsPointTest, ContainsReturnsTrueForLastPointBeforeExclusiveRightBottom)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_TRUE(rect.contains(spk::Vector2Int(39, 59)));
}

TEST(Rect2DContainsPointTest, ContainsReturnsFalseForRightEdge)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_FALSE(rect.contains(spk::Vector2Int(40, 25)));
}

TEST(Rect2DContainsPointTest, ContainsReturnsFalseForBottomEdge)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_FALSE(rect.contains(spk::Vector2Int(15, 60)));
}

TEST(Rect2DContainsPointTest, ContainsReturnsFalseForPointLeftOfRect)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_FALSE(rect.contains(spk::Vector2Int(9, 25)));
}

TEST(Rect2DContainsPointTest, ContainsReturnsFalseForPointAboveRect)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_FALSE(rect.contains(spk::Vector2Int(15, 19)));
}

TEST(Rect2DContainsRectTest, ContainsReturnsTrueForIdenticalRect)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	EXPECT_TRUE(rect.contains(rect));
}

TEST(Rect2DContainsRectTest, ContainsReturnsTrueForFullyContainedRect)
{
	const spk::Rect2D outer(10, 20, 30, 40);
	const spk::Rect2D inner(15, 25, 10, 10);

	EXPECT_TRUE(outer.contains(inner));
}

TEST(Rect2DContainsRectTest, ContainsReturnsFalseForPartiallyOverlappingRect)
{
	const spk::Rect2D outer(10, 20, 30, 40);
	const spk::Rect2D other(35, 25, 20, 10);

	EXPECT_FALSE(outer.contains(other));
}

TEST(Rect2DContainsRectTest, ContainsReturnsFalseForRectOutside)
{
	const spk::Rect2D outer(10, 20, 30, 40);
	const spk::Rect2D other(100, 200, 10, 10);

	EXPECT_FALSE(outer.contains(other));
}

TEST(Rect2DTranslationTest, TranslatedMovesAnchorAndPreservesSize)
{
	const spk::Rect2D rect(10, 20, 30, 40);
	const spk::Rect2D result = rect.translated(spk::Vector2Int(5, -7));

	EXPECT_EQ(result.anchor.x, 15);
	EXPECT_EQ(result.anchor.y, 13);
	EXPECT_EQ(result.size.x, 30u);
	EXPECT_EQ(result.size.y, 40u);
}

TEST(Rect2DShrinkTest, ShrinkReducesBoundsSymmetrically)
{
	const spk::Rect2D rect(10, 20, 30, 40);
	const spk::Rect2D result = rect.shrink(spk::Vector2Int(2, 3));

	EXPECT_EQ(result.anchor.x, 12);
	EXPECT_EQ(result.anchor.y, 23);
	EXPECT_EQ(result.size.x, 26u);
	EXPECT_EQ(result.size.y, 34u);
}

TEST(Rect2DShrinkTest, ShrinkCanCollapseRectToEmpty)
{
	const spk::Rect2D rect(10, 20, 4, 6);
	const spk::Rect2D result = rect.shrink(spk::Vector2Int(10, 10));

	EXPECT_TRUE(result.empty());
}

TEST(Rect2DIntersectTest, IntersectReturnsOverlappingRegion)
{
	const spk::Rect2D left(10, 20, 30, 40);
	const spk::Rect2D right(25, 35, 50, 10);

	const spk::Rect2D result = left.intersect(right);

	EXPECT_EQ(result.anchor.x, 25);
	EXPECT_EQ(result.anchor.y, 35);
	EXPECT_EQ(result.size.x, 15u);
	EXPECT_EQ(result.size.y, 10u);
}

TEST(Rect2DIntersectTest, IntersectReturnsSelfForIdenticalRects)
{
	const spk::Rect2D rect(10, 20, 30, 40);
	const spk::Rect2D result = rect.intersect(rect);

	EXPECT_EQ(result, rect);
}

TEST(Rect2DIntersectTest, IntersectReturnsEmptyRectWhenNoOverlap)
{
	const spk::Rect2D left(10, 20, 30, 40);
	const spk::Rect2D right(100, 200, 10, 10);

	const spk::Rect2D result = left.intersect(right);

	EXPECT_TRUE(result.empty());
}

TEST(Rect2DIntersectTest, IntersectReturnsEmptyRectWhenOnlyTouchingEdges)
{
	const spk::Rect2D left(10, 20, 30, 40);
	const spk::Rect2D right(40, 20, 10, 10);

	const spk::Rect2D result = left.intersect(right);

	EXPECT_TRUE(result.empty());
}

TEST(Rect2DStringTest, StreamOutputMatchesExpectedFormat)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	std::ostringstream outputStream;
	outputStream << rect;

	EXPECT_EQ(outputStream.str(), "{ anchor: (10, 20), size: (30, 40) }");
}

TEST(Rect2DStringTest, ToStringMatchesStreamOutput)
{
	const spk::Rect2D rect(10, 20, 30, 40);

	std::ostringstream outputStream;
	outputStream << rect;

	EXPECT_EQ(rect.toString(), outputStream.str());
}

TEST(Rect2DEqualityTest, EqualityReturnsTrueForSameValues)
{
	const spk::Rect2D left(10, 20, 30, 40);
	const spk::Rect2D right(10, 20, 30, 40);

	EXPECT_TRUE(left == right);
	EXPECT_FALSE(left != right);
}

TEST(Rect2DEqualityTest, EqualityReturnsFalseWhenAnchorDiffers)
{
	const spk::Rect2D left(10, 20, 30, 40);
	const spk::Rect2D right(11, 20, 30, 40);

	EXPECT_FALSE(left == right);
	EXPECT_TRUE(left != right);
}

TEST(Rect2DEqualityTest, EqualityReturnsFalseWhenSizeDiffers)
{
	const spk::Rect2D left(10, 20, 30, 40);
	const spk::Rect2D right(10, 20, 31, 40);

	EXPECT_FALSE(left == right);
	EXPECT_TRUE(left != right);
}
#include <string>
#include <iostream>


#include "gtest/gtest.h"
#include "BoundingBox.h"

TEST(BoundingBox, CoordinateSize){
	Point lb(1,2,0);
	Point ub(2,4,0);
	BoundingBox bb1(lb, ub);

	EXPECT_EQ(2, bb1.sizeCoordX());
	EXPECT_EQ(3, bb1.sizeCoordY());
	EXPECT_EQ(1, bb1.sizeCoordZ());

	Point lb2(1,2,0);
	Point ub2(2,4,0);

	BoundingBox bb2(lb, ub);
	// Intersection should return an bounding box equal to bb2 and bb1,
	// they are the same bb
	BoundingBox bbIntersection = bb2.intersection(bb1);
	EXPECT_TRUE( bbIntersection.getLb().getX() == bb2.getLb().getX() &&
				bbIntersection.getLb().getY() == bb2.getLb().getY() &&
				bbIntersection.getLb().getZ() == bb2.getLb().getZ());

	EXPECT_TRUE( bbIntersection.getUb().getX() == bb2.getUb().getX() &&
				bbIntersection.getUb().getY() == bb2.getUb().getY() &&
				bbIntersection.getUb().getZ() == bb2.getUb().getZ());

	Point lb3(2,3,0);
	Point ub3(2,4,0);
	BoundingBox bb3(lb3, ub3);
	bbIntersection = bb2.intersection(bb3);
	// lower bound is the value of of bb3
	EXPECT_TRUE( bbIntersection.getLb().getX() == lb3.getX() &&
				bbIntersection.getLb().getY() == lb3.getY() &&
				bbIntersection.getLb().getZ() == lb3.getZ());

	// upper bound should not change
	EXPECT_TRUE( bbIntersection.getUb().getX() == bb2.getUb().getX() &&
				bbIntersection.getUb().getY() == bb2.getUb().getY() &&
				bbIntersection.getUb().getZ() == bb2.getUb().getZ());
}

TEST(BoundingBox, Intersection){
	Point lb(1,2,0);
	Point ub(2,4,0);
	BoundingBox bb1(lb, ub);
	BoundingBox bb2(lb, ub);

	// Intersection should return an bounding box equal to bb2 and bb1,
	// they are the same bb
	BoundingBox bbIntersection = bb2.intersection(bb1);
	EXPECT_TRUE( bbIntersection.getLb().getX() == bb2.getLb().getX() &&
				bbIntersection.getLb().getY() == bb2.getLb().getY() &&
				bbIntersection.getLb().getZ() == bb2.getLb().getZ());

	EXPECT_TRUE( bbIntersection.getUb().getX() == bb2.getUb().getX() &&
				bbIntersection.getUb().getY() == bb2.getUb().getY() &&
				bbIntersection.getUb().getZ() == bb2.getUb().getZ());

	Point lb3(2,3,0);
	Point ub3(2,4,0);
	BoundingBox bb3(lb3, ub3);
	bbIntersection = bb2.intersection(bb3);
	// lower bound is the value of of bb3
	EXPECT_TRUE( bbIntersection.getLb().getX() == lb3.getX() &&
				bbIntersection.getLb().getY() == lb3.getY() &&
				bbIntersection.getLb().getZ() == lb3.getZ());

	// upper bound should not change
	EXPECT_TRUE( bbIntersection.getUb().getX() == bb2.getUb().getX() &&
				bbIntersection.getUb().getY() == bb2.getUb().getY() &&
				bbIntersection.getUb().getZ() == bb2.getUb().getZ());

}


TEST(BoundingBox, NoIntersection){
	Point lb(1,2,0), ub(2,4,0), lb2(3,5,0), ub2(4,6,0);
	BoundingBox bb1(lb, ub);
	BoundingBox bb2(lb2, ub2);

	// Intersection should return an bounding box equal to bb2 and bb1,
	// they are the same bb
	BoundingBox bbInter = bb2.intersection(bb1);
	EXPECT_FALSE( bb1.doesIntersect(bb2));

	// Lower bound of intersection should contain greater values
	// among lower bound of both bounding boxes (lb2)
	EXPECT_TRUE( bbInter.getLb().getX() == lb2.getX() &&
				bbInter.getLb().getY() == lb2.getY() &&
				bbInter.getLb().getZ() == lb2.getZ());

	// Upper bound of intersection should contain smaller values
	// among upper bound of both bounding boxes (ub)
	EXPECT_TRUE( bbInter.getUb().getX() == ub.getX() &&
				bbInter.getUb().getY() == ub.getY() &&
				bbInter.getUb().getZ() == ub.getZ());

}

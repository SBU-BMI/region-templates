
#include <string>
#include <iostream>
/*
#include "RegionTemplate.h"
#include "ImageFileChunksDataSource.h"
#include "gtest/gtest.h"
#include "DenseDataRegion2D.h"
#include "Constants.h"
#include "ImageFileChunksDataSource.h"


TEST(RegionTemplates, Instantitate){
	RegionTemplate rt;
	EXPECT_EQ(std::string(""), rt.getName());

	DataRegion *dr = rt.getDataRegion("", 0, 0);
	EXPECT_TRUE(dr == NULL);

}

TEST(RegionTemplates, Read2DDenseRegion){
	RegionTemplate *rt = new RegionTemplate();
	// Data source to create 2D Dense Region Template from files folder
	ImageFileChunksDataSource *ds = new ImageFileChunksDataSource( "data/color", "tif", 100, "data/color/out");

	//
	DenseDataRegion2D *ddr2D = new DenseDataRegion2D();
	ddr2D->setInBds(ds);
	ddr2D->setOutBds(ds);
	ddr2D->setName("FirstImage");

	// I know before hand that this is the actual domain of the data.
	BoundingBox bb(Point(0,0,0),Point(99,99,0));
	ddr2D->setBb(bb);

	// Create data region from file chunks. Read the actual data in memory.
	ddr2D->instantiateRegion();

	std::string dataRegionName = ddr2D->getName();
	EXPECT_STREQ("FirstImage", dataRegionName.c_str());

	rt->insertDataRegion(ddr2D);
	DenseDataRegion2D *ddr2D_2 = dynamic_cast<DenseDataRegion2D*>(rt->getDataRegion(dataRegionName, 0, 0));
	EXPECT_EQ(ddr2D, ddr2D_2);

	// Retrieving region with wrong version
	ddr2D_2 = dynamic_cast<DenseDataRegion2D*>(rt->getDataRegion(dataRegionName, 1, 0));
	EXPECT_EQ(NULL, ddr2D_2);

	// Retrieving region with wrong timestamp
	ddr2D_2 = dynamic_cast<DenseDataRegion2D*>(rt->getDataRegion(dataRegionName, 0, 1));
	EXPECT_EQ(NULL, ddr2D_2);
	delete rt;

}


TEST(Region, Read2DDenseRegion){
	// Data source to create 2D Dense Region Template from files folder
	ImageFileChunksDataSource *ds = new ImageFileChunksDataSource( "data/color", "tif", 100, "data/color/out");

	//
	DenseDataRegion2D *ddr2D = new DenseDataRegion2D();
	ddr2D->setInBds(ds);
	ddr2D->setOutBds(ds);
	ddr2D->setName("FirstImage");

	// I know before hand that this is the actual domain of the data.
	BoundingBox bb(Point(0,0,0),Point(99,99,0));
	ddr2D->setBb(bb);

	// Create data region from file chunks. Read the actual data in memory.
	ddr2D->instantiateRegion();

	// Make sure the data type is set correctly
//	EXPECT_EQ(RegionTemplateType::DENSE_REGION_2D, dr->getType());
	ASSERT_EQ(1, ddr2D->getType());

	// Check where the size of the data in memory matches the bounding box size
	EXPECT_EQ(bb.sizeCoordX(), ddr2D->getXDimensionSize());
	EXPECT_EQ(bb.sizeCoordY(), ddr2D->getYDimensionSize());

	// writting entire region to disk
	ddr2D->write();

	cv::Mat groundTruth = cv::imread("data/color/0-0.tif",-1);

	ASSERT_EQ(ddr2D->getXDimensionSize(), groundTruth.cols);
	ASSERT_EQ(ddr2D->getYDimensionSize(), groundTruth.rows);

	for(int i = 0; i < groundTruth.rows; i++)
	{
	    unsigned char* GTi = groundTruth.ptr<unsigned char>(i);
	    unsigned char* DDRi = ddr2D->getData().ptr<unsigned char>(i);
	    for(int j = 0; j < groundTruth.cols; j++)
	    	EXPECT_EQ(GTi[j], DDRi[j]);
	}


	std::string dataRegionName = ddr2D->getName();
	EXPECT_STREQ("FirstImage", dataRegionName.c_str());

	delete ddr2D;

}*/

/*TEST(RegionTemplates, enganation){
	cv::Mat chunkData = cv::imread("data/0-0.png", -1);

	cv::Rect intersectionInROI(3996, 3996, 100, 100 );
	cv::Mat chunkDataRoi(chunkData, intersectionInROI);
	cv::imwrite("chunk.tif", chunkDataRoi);

}*/

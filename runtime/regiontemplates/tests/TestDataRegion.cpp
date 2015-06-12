
#include <string>
#include <iostream>

#include "RegionTemplate.h"
//#include "ImageFileChunksDataSource.h"
#include "gtest/gtest.h"
#include "DenseDataRegion2D.h"
#include "Constants.h"
//#include "ImageFileChunksDataSource.h"

int main(int argc, char **argv){

	DataRegion *dr = new DataRegion();
	dr->setName("drTest");
	dr->setId("drInputFile.png");
	dr->setTimestamp(1);
	dr->setVersion(2);
	dr->setBb(BoundingBox(Point(0,0,0),Point(360,240,0)));
	dr->setROI(BoundingBox(Point(5,5,0),Point(15,15,0)));
	BoundingBox bb1(Point(0,0,0), Point(359,239,0));
	BoundingBox bb2(Point(360,240,0), Point(719,479,0));
	BoundingBox bb3(Point(0,240,0), Point(359,479,0));
	BoundingBox bb4(Point(360,0,0), Point(719,239,0));
	std::string fileName1("00000001.png");
	std::string fileName2("00000001.png");
	std::string fileName3("00000001.png");
	std::string fileName4("00000001.png");
	dr->insertBB2IdElement(bb1, fileName1);
	dr->insertBB2IdElement(bb2, fileName2);
	dr->insertBB2IdElement(bb3, fileName3);
	dr->insertBB2IdElement(bb4, fileName4);
	char *buff = (char*)malloc(sizeof(char)*dr->serializationSize());
	std::cout << "Buffer Size: "<< dr->serializationSize() << std::endl;
	std::cout << "DATA REGION BEFORE SERIALIZE " << std::endl;
	dr->serialize(buff);
	dr->print();

	std::cout << "DATA REGION AFTER DESERIALIZE " << std::endl;
	DataRegion* dr2 = new DataRegion();
	dr2->deserialize(buff);
	dr2->print();
free(buff);
delete dr;
delete dr2;
	DenseDataRegion2D* dr2D = new DenseDataRegion2D();
	dr2D->insertBB2IdElement(bb1, fileName1);
	dr2D->insertBB2IdElement(bb2, fileName2);
	dr2D->insertBB2IdElement(bb3, fileName3);
	dr2D->insertBB2IdElement(bb4, fileName4);

	cv::Mat* retrievedData = dr2D->getData(100,100,100,100, true);
//	cv::namedWindow("BB");
//	cv::imshow("BB", *retrievedData);
//	cv::waitKey();

	cv::Mat* retrievedData2 = dr2D->getData(0,0,720,480, true);
//	cv::namedWindow("BB2");
//	cv::imshow("BB2", *retrievedData2);
//	cv::waitKey();

	cv::Mat* retrievedData3 = dr2D->getData(300,200,120,80, true);
//	cv::namedWindow("BB3");
//	cv::imshow("BB3", *retrievedData3);
//	cv::waitKey();
	dr2D->setROI(BoundingBox(Point(0,0,0), Point(719,479,0)));
	cv::Mat* retrievedData4 = dr2D->getData(-1, -1, -1, -1);
	cv::namedWindow("BB4");
	cv::imshow("BB4", *retrievedData4);
	cv::waitKey();

	delete retrievedData;
	delete retrievedData2;
	delete retrievedData3;


	delete dr2D;


	return 0;

}


#include <string>
#include <iostream>

#include "RegionTemplate.h"
//#include "ImageFileChunksDataSource.h"
#include "gtest/gtest.h"
#include "DenseDataRegion2D.h"
#include "Constants.h"
//#include "ImageFileChunksDataSource.h"

int main(int argc, char **argv){
//	RegionTemplate *rt = new RegionTemplate();
	// Data source to create 2D Dense Region Template from files folder
//	ImageFileChunksDataSource *ds = new ImageFileChunksDataSource( "data/color", "tif", 100, "data/color/out");

	//
	DenseDataRegion2D *ddr2D = new DenseDataRegion2D();
	ddr2D->setId("FirstImage");

	DataRegion * dr = ddr2D;


	delete dr;

}

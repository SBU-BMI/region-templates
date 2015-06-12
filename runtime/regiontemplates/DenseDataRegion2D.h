/*
 * DenseDataRegion2D.h
 *
 *  Created on: Oct 22, 2012
 *      Author: george
 */

#ifndef DENSEDATAREGION2D_H_
#define DENSEDATAREGION2D_H_

#include "DataRegion.h"
#include <string>
#include <iostream>

// OpenCV library includes
#include "cv.hpp"

//#include <cv.h>
#include <highgui.h>
#include <vector>

//using namespace cv;
#include "opencv2/gpu/gpu.hpp"

//class DataRegionFactory;

class DenseDataRegion2D: public DataRegion {
private:
	cv::Mat dataCPU;
	cv::gpu::GpuMat dataGPU;

	// used to store chunks of data loaded in memory
	std::map<BoundingBox, cv::Mat, BBComparator > chunkedDataCaching;

public:
	DenseDataRegion2D();
	virtual ~DenseDataRegion2D();

	DataRegion* clone(bool copyData);

	void allocData(int x, int y, int type, int device=Environment::CPU);
	void releaseData(int device=Environment::CPU);
	void setData(cv::Mat data);

	void insertChukedData(BoundingBox, cv::Mat);
	bool loadChunkToCache(int chunkIndex);
	void deleteChunkFromCache(BoundingBox);

	cv::Mat getData();
	cv::Mat *getData(int x, int y, int width, int height, bool cacheData=false);

	bool empty();
	long getDataSize();


	int getXDimensionSize();
	int getYDimensionSize();
};

#endif /* DENSEDATAREGION2D_H_ */

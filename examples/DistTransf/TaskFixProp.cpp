/*
 * TaskDoAll.cpp
 *
 *  Created on: Mar 3, 2012
 *      Author: george
 */

#include "TaskFixProp.h"

TaskFixProp::TaskFixProp(::cv::Mat* neighbor, ::cv::Mat* originalMaskImage , int tileSize) {
	this->neighbor = neighbor;
	this->originalMaskImage = originalMaskImage;
	this->tileSize = tileSize;
}

TaskFixProp::~TaskFixProp() {

}

bool TaskFixProp::run(int procType, int tid) {

	uint64_t t0 = Util::ClockGetTimeProfile();

	std::cout << "neighbor.cols: "<< (*neighbor).cols << std::endl;
	Mat distanceMap = nscale::distTransformFixTilingEffects(*neighbor, tileSize, false);

	uint64_t t1 = Util::ClockGetTimeProfile();
	std::cout << "Fix tiling effect: procType:"<< procType<<" time="<<t1-t0<<std::endl;

//	Mat distOriginal = nscale::distanceTransform(*this->originalMaskImage, false);
//	uint64_t t3 = Util::ClockGetTimeProfile();

//	std::cout << "diff - sequential vs. tiled:"<< countNonZero(distanceMap != distOriginal) << std::endl;

}



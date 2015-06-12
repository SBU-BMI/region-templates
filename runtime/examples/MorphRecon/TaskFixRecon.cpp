/*
 * TaskDoAll.cpp
 *
 *  Created on: Mar 3, 2012
 *      Author: george
 */

#include "TaskFixRecon.h"

TaskFixRecon::TaskFixRecon(::cv::Mat* markerImage, ::cv::Mat* originalMarkerImage , ::cv::Mat* maskImage, int tileSize) {
	this->markerImage = markerImage;
	this->maskImage = maskImage;
	this->originalMarkerImage = originalMarkerImage;

	this->tileSize = tileSize;
}

TaskFixRecon::~TaskFixRecon() {

}

bool TaskFixRecon::run(int procType, int tid) {

	uint64_t t0 = Util::ClockGetTimeProfile();

	Mat reconCopy = nscale::imreconstructFixTilingEffects<unsigned char>(*markerImage, *maskImage, 4,0, 0, tileSize, true);
	//Mat reconCopy = nscale::imreconstructFixTilingEffectsParallel<unsigned char>(*markerImage, *maskImage, 8, tileSize, true);

	uint64_t t1 = Util::ClockGetTimeProfile();
	std::cout << "Fix tiling effect: procType:"<< procType<<" time="<<t1-t0<<std::endl;

//	t1 = cciutils::ClockGetTime();
//	Mat recon = nscale::imreconstruct<unsigned char>(*originalMarkerImage, *maskImage, 8);
//	t2 = cciutils::ClockGetTime();

//	Mat comp = recon != reconCopy;
//	std::cout << "comp diff= "<<countNonZero(comp) << std::endl;
//	imwrite("diff.ppm", comp);
//	imwrite("recon.ppm", recon);
}



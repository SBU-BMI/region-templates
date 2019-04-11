#include "TaskFeatures.h"

TaskFeatures::TaskFeatures(DenseDataRegion2D* bgr, DenseDataRegion2D* mask) {
	this->bgr = bgr;
	this->mask = mask;
}

TaskFeatures::~TaskFeatures() {
}

bool TaskFeatures::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();

	cv::Mat inputImage = this->bgr->getData();
	cv::Mat maskImage = this->mask->getData();
	std::cout << "nChannels:  "<< maskImage.channels() << std::endl;
	if(inputImage.rows > 0 && maskImage.rows > 0)
		nscale::ObjFeatures::calcFeatures(inputImage, maskImage);
	else
		std::cout << "Not Computing features" << std::endl;

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Feature Computation time elapsed: "<< t2-t1 << std::endl;

	return true;
}

#include "TaskFeatures.h"

TaskFeatures::TaskFeatures(DenseDataRegion2D* bgr, DenseDataRegion2D* mask) {
	this->bgr = bgr;
	this->mask = mask;
}

TaskFeatures::~TaskFeatures() {
	if(bgr != NULL) delete bgr;
	if(mask != NULL) delete mask;
}

bool TaskFeatures::run(int procType, int tid) {
	int *bbox = NULL;
	int compcount;
	uint64_t t1 = Util::ClockGetTimeProfile();

	cv::Mat inputImage = this->bgr->getData();
	cv::Mat maskImage = this->mask->getData();
	std::cout << "nChannels:  "<< maskImage.channels() << std::endl;
	if(inputImage.rows > 0 && maskImage.rows > 0)
		//nscale::ObjFeatures::calcFeatures(inputImage, maskImage);
		std::cout << "FEATURE COMPUTATION COMES HERE!!" << std::endl;
	else
		std::cout << "Not Computing features" << std::endl;

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Feature Computation time elapsed: "<< t2-t1 << std::endl;
}

#include "TaskSegmentation.h"

TaskSegmentation::TaskSegmentation(DenseDataRegion2D* bgr, DenseDataRegion2D* mask, float otsuRatio, float curvatureWeight, float sizeThld, float sizeUpperThld) {
	this->bgr = bgr;
	this->mask = mask;
	this->otsuRatio = otsuRatio;
	this->curvatureWeight = curvatureWeight;
	this->sizeThld = sizeThld;
	this->sizeUpperThld = sizeUpperThld;
}

TaskSegmentation::~TaskSegmentation() {
	if(bgr != NULL) delete bgr;
}

bool TaskSegmentation::run(int procType, int tid) {
	cv::Mat inputImage = this->bgr->getData();
	cv::Mat outMask;

	uint64_t t1 = Util::ClockGetTimeProfile();


//	cv::Mat seg = processTile(thisTile, otsuRatio, curvatureWeight, sizeThld, sizeUpperThld, mpp);
	if(inputImage.rows > 0)
		outMask = processTile(inputImage, otsuRatio, curvatureWeight, sizeThld, sizeUpperThld, 0.25);
	else
		std::cout <<"Segmentation: input data NULL"<< std::endl;
	this->mask->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

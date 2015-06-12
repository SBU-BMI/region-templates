#include "TaskSegmentation.h"

TaskSegmentation::TaskSegmentation(DenseDataRegion2D* bgr, DenseDataRegion2D* mask) {
	this->bgr = bgr;
	this->mask = mask;
}

TaskSegmentation::~TaskSegmentation() {
	if(bgr != NULL) delete bgr;
}

bool TaskSegmentation::run(int procType, int tid) {
	cv::Mat inputImage = this->bgr->getData();
	cv::Mat outMask;

//	std::cout << "Start Task Segmentation. inputImage.rows: "<< inputImage.rows << std::endl;
//	std::cout << "\t\t inputImageAddr: " << &inputImage << std::endl;
	uint64_t t1 = Util::ClockGetTimeProfile();

	int segmentationExecCode = ::nscale::HistologicalEntities::segmentNuclei(inputImage, outMask);

	this->mask->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

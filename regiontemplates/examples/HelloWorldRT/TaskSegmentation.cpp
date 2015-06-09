#include "TaskSegmentation.h"

TaskSegmentation::TaskSegmentation(DenseDataRegion2D* bgr, DenseDataRegion2D* mask) {
	this->bgr = bgr;
	this->mask = mask;
}

TaskSegmentation::~TaskSegmentation() {
}

bool TaskSegmentation::run(int procType, int tid) {
	cv::Mat inputImage = this->bgr->getData();
	cv::Mat outMask;

	this->bgr->setInputType(DataSourceType::DATA_SPACES);

//	std::cout << "Start Task Segmentation. inputImage.rows: "<< inputImage.rows << std::endl;
//	std::cout << "\t\t inputImageAddr: " << &inputImage << std::endl;
	uint64_t t1 = Util::ClockGetTimeProfile();

	int segmentationExecCode = ::nscale::HistologicalEntities::segmentNuclei(inputImage, outMask);

	if(segmentationExecCode != ::nscale::HistologicalEntities::CONTINUE){
		std::cout << "Segmentation code is not continue" << std::endl;
	}
	this->mask->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1  << " bgr->id:" << this->bgr->getId() << std::endl;
}

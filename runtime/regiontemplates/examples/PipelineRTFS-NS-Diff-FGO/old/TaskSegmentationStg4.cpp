#include "TaskSegmentationStg4.hpp"

TaskSegmentationStg4::TaskSegmentationStg4(const cv::Mat* img, int minSizePl, int watershedConnectivity, 
		cv::Mat* seg_open, cv::Mat* seg_nonoverlap, ::cciutils::SimpleCSVLogger *logger, 
		::cciutils::cv::IntermediateResultHandler *iresHandler){

	this->img = img;
	this->minSizePl = minSizePl;
	this->watershedConnectivity = watershedConnectivity;
	this->seg_open = seg_open;
	this->seg_nonoverlap = seg_nonoverlap;

	this->logger = logger;
	this->iresHandler = iresHandler;
}

TaskSegmentationStg4::~TaskSegmentationStg4() {}

bool TaskSegmentationStg4::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentationStg4:" << std::endl;
		int segmentationExecCode = ::nscale::HistologicalEntities::segmentNucleiStg4(img, minSizePl, 
			watershedConnectivity, seg_open, seg_nonoverlap, logger, iresHandler);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

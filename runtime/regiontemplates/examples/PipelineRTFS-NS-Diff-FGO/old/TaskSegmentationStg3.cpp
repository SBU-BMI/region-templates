#include "TaskSegmentationStg3.hpp"

TaskSegmentationStg3::TaskSegmentationStg3(cv::Mat* seg_nohole, cv::Mat* seg_open, 
		::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler){

	this->seg_open = seg_open;
	this->seg_nohole = seg_nohole;
	this->logger = logger;
	this->iresHandler = iresHandler;
}

TaskSegmentationStg3::~TaskSegmentationStg3() {}

bool TaskSegmentationStg3::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentationStg3:" << std::endl;
		int segmentationExecCode = ::nscale::HistologicalEntities::segmentNucleiStg3(seg_nohole, seg_open, logger, iresHandler);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

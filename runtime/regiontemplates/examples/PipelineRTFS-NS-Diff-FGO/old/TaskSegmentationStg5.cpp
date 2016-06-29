#include "TaskSegmentationStg5.hpp"

TaskSegmentationStg5::TaskSegmentationStg5(int minSizeSeg, int maxSizeSeg, cv::Mat* seg_nonoverlap, cv::Mat* seg, 
		::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler){

	this->minSizeSeg = minSizeSeg;
	this->maxSizeSeg = maxSizeSeg;
	this->seg_nonoverlap = seg_nonoverlap;
	this->seg = seg;

	this->logger = logger;
	this->iresHandler = iresHandler;
}

TaskSegmentationStg5::~TaskSegmentationStg5() {}

bool TaskSegmentationStg5::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentationStg5:" << std::endl;
		int segmentationExecCode = ::nscale::HistologicalEntities::segmentNucleiStg5(minSizeSeg, maxSizeSeg, seg_nonoverlap, 
			seg, logger, iresHandler);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

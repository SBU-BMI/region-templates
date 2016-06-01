#include "TaskSegmentationStg2.hpp"

TaskSegmentationStg2::TaskSegmentationStg2(int* findCandidateResult, cv::Mat* seg_norbc, cv::Mat* seg_nohole, 
		::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler){

	this->findCandidateResult = findCandidateResult;
	this->seg_norbc = seg_norbc;
	this->seg_nohole = seg_nohole;
	this->logger = logger;
	this->iresHandler = iresHandler;
}

TaskSegmentationStg2::~TaskSegmentationStg2() {}

bool TaskSegmentationStg2::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentationStg2:" << std::endl;
		int segmentationExecCode = ::nscale::HistologicalEntities::segmentNucleiStg2(findCandidateResult, seg_norbc, seg_nohole, logger, iresHandler);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

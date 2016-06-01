#include "TaskSegmentationStg6.hpp"

TaskSegmentationStg6::TaskSegmentationStg6(cv::Mat* seg, DenseDataRegion2D* output, int fillHolesConnectivity, ::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler){

	this->seg = seg;
	this->output = output;
	this->fillHolesConnectivity = fillHolesConnectivity;

	this->logger = logger;
	this->iresHandler = iresHandler;
}

TaskSegmentationStg6::~TaskSegmentationStg6() {}

bool TaskSegmentationStg6::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();
	cv::Mat outMask;

	std::cout << "TaskSegmentationStg6:" << std::endl;
		int segmentationExecCode = ::nscale::HistologicalEntities::segmentNucleiStg6(&outMask, fillHolesConnectivity, seg, logger, iresHandler);

	this->output->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

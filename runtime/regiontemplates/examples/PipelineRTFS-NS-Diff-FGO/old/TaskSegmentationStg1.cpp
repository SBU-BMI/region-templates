#include "TaskSegmentationStg1.hpp"

TaskSegmentationStg1::TaskSegmentationStg1(DenseDataRegion2D* bgr, unsigned char blue, unsigned char green, unsigned char red, double T1, double T2, unsigned char G1, int minSize, int maxSize, unsigned char G2, int fillHolesConnectivity, int reconConnectivity, int* findCandidateResult, cv::Mat* seg_norbc, ::cciutils::SimpleCSVLogger *logger, ::cciutils::cv::IntermediateResultHandler *iresHandler) {

	this->img = bgr->getData();

	this->blue = blue;
	this->green = green;
	this->red = red;

	this->T1 = T1;
	this->T2 = T2;
	this->G1 = G1;
	this->G2 = G2;
	this->minSize = minSize;
	this->maxSize = maxSize;
	this->fillHolesConnectivity=fillHolesConnectivity;
	this->reconConnectivity=reconConnectivity;

	this->seg_norbc = seg_norbc;
	this->findCandidateResult = findCandidateResult;
	this->logger = logger;
	this->iresHandler = iresHandler;
}

TaskSegmentationStg1::~TaskSegmentationStg1() {};

bool TaskSegmentationStg1::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentationStg1" << std::endl;
	int segmentationExecCode = ::nscale::HistologicalEntities::segmentNucleiStg1(img, blue, green, red, T1, T2, G1, minSize, maxSize, G2, fillHolesConnectivity, reconConnectivity, findCandidateResult, seg_norbc, logger, iresHandler);
	
	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation Stg1 time elapsed: "<< t2-t1 << std::endl;
}

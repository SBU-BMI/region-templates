#include "TaskSegmentation.h"

TaskSegmentation::TaskSegmentation(DenseDataRegion2D* bgr, DenseDataRegion2D* mask, unsigned char blue, unsigned char green, unsigned char red, double T1, double T2, unsigned char G1, unsigned char G2, int minSize, int maxSize, int minSizePl, int minSizeSeg, int maxSizeSeg, int fillHolesConnectivity, int reconConnectivity, int watershedConnectivity) {
	this->bgr = bgr;
	this->mask = mask;

	this->blue = blue;
	this->green = green;
	this->red = red;

	this->T1 = T1;
	this->T2 = T2;
	this->G1 = G1;
	this->G2 = G2;
	this->minSize = minSize;
	this->maxSize = maxSize;
	this->minSizePl = minSizePl;
	this->minSizeSeg = minSizeSeg;
	this->maxSizeSeg = maxSizeSeg;
	this->fillHolesConnectivity=fillHolesConnectivity;
	this->reconConnectivity=reconConnectivity;
	this->watershedConnectivity=watershedConnectivity;
}

TaskSegmentation::~TaskSegmentation() {
	if(bgr != NULL) delete bgr;
}

bool TaskSegmentation::run(int procType, int tid) {
	cv::Mat inputImage = this->bgr->getData();
	cv::Mat outMask;

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentation: " << (int)blue << ":"<< (int)green << ":"<< (int)red << ":"<< T1<< ":"<< T2<< ":"<< (int)G1<< ":"<< minSize<< ":"<< maxSize<< ":"<< (int)G2<< ":"<< minSizePl<< ":"<< minSizeSeg<< ":"<< maxSizeSeg << ":" << fillHolesConnectivity << ":" << reconConnectivity << ":" << watershedConnectivity << std::endl;
	if(inputImage.rows > 0)
		int segmentationExecCode = ::nscale::HistologicalEntities::segmentNuclei(inputImage, outMask, blue, green, red, T1, T2, G1, minSize, maxSize, G2, minSizePl, minSizeSeg, maxSizeSeg, fillHolesConnectivity, reconConnectivity, watershedConnectivity);
	else
		std::cout <<"Segmentation: input data NULL"<< std::endl;
	this->mask->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

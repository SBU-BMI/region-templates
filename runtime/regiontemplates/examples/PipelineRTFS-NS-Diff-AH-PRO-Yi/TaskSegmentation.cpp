#include "TaskSegmentation.h"

TaskSegmentation::TaskSegmentation(DenseDataRegion2D* bgr, DenseDataRegion2D* mask, unsigned char blue) {
	this->bgr = bgr;
	this->mask = mask;

	this->blue = blue;
}

TaskSegmentation::~TaskSegmentation() {
	if(bgr != NULL) delete bgr;
}

bool TaskSegmentation::run(int procType, int tid) {
	cv::Mat inputImage = this->bgr->getData();
	cv::Mat outMask;

	uint64_t t1 = Util::ClockGetTimeProfile();


//	cv::Mat seg = processTile(thisTile, otsuRatio, curvatureWeight, sizeThld, sizeUpperThld, mpp);
//	std::cout << "TaskSegmentation: " << (int)blue << ":"<< (int)green << ":"<< (int)red << ":"<< T1<< ":"<< T2<< ":"<< (int)G1<< ":"<< minSize<< ":"<< maxSize<< ":"<< (int)G2<< ":"<< minSizePl<< ":"<< minSizeSeg<< ":"<< maxSizeSeg << ":" << fillHolesConnectivity << ":" << reconConnectivity << ":" << watershedConnectivity << std::endl;
	if(inputImage.rows > 0)
		outMask = processTile(inputImage, 1.0, 0.8, 3, 200, 0.25);
//		int segmentationExecCode = ::nscale::HistologicalEntities::segmentNuclei(inputImage, outMask, blue, green, red, T1, T2, G1, minSize, maxSize, G2, minSizePl, minSizeSeg, maxSizeSeg, fillHolesConnectivity, reconConnectivity, watershedConnectivity);
	else
		std::cout <<"Segmentation: input data NULL"<< std::endl;
	this->mask->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

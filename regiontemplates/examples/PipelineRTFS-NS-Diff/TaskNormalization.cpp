#include "TaskNormalization.h"

TaskNormalization::TaskNormalization(DenseDataRegion2D* raw, DenseDataRegion2D* bgr, int paramId) {
	this->bgr = bgr;
	this->raw = raw;
	this->paramId = paramId;
}

TaskNormalization::~TaskNormalization() {
}

bool TaskNormalization::run(int procType, int tid) {
	cv::Mat inputImage = this->raw->getData();
	// target values computed from the reference image
	float targetMean[3] = {-0.576, -0.0233, 0.0443};
	float targetStd[3] = {0.2317, 0.0491, 0.0156};
	if(paramId == 1){
		targetMean[0] = -0.376;
		targetMean[1] = -0.0133;
		targetMean[2] = 0.0243;
	}
	//float targetMean[3] = {-0.376, -0.0133, 0.0243};
	cv::Mat NormalizedImg;
	std::cout << "Normalization::: paramId: " << paramId<< std::endl;
	uint64_t t1 = Util::ClockGetTimeProfile();
	if(inputImage.rows > 0)
		NormalizedImg = ::nscale::Normalization::normalization(inputImage, targetMean, targetStd);
	else
		std::cout <<"Normalization: input data NULL" << std::endl;
	uint64_t t2 = Util::ClockGetTimeProfile();

	this->bgr->setData(NormalizedImg);

	std::cout << "Task Normalization time elapsed: "<< t2-t1 << std::endl;
}

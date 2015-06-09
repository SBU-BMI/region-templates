#include "TaskFeatures.h"

TaskFeatures::TaskFeatures(DenseDataRegion2D* bgr, DenseDataRegion2D* mask, DenseDataRegion2D* features) {
	this->bgr = bgr;
	this->mask = mask;
	this->features = features;
}

TaskFeatures::~TaskFeatures() {
}

bool TaskFeatures::run(int procType, int tid) {
	uint64_t t1 = Util::ClockGetTimeProfile();

	cv::Mat inputImage = this->bgr->getData();
	cv::Mat maskImage = this->mask->getData();
	std::cout << "nChannels:  "<< maskImage.channels() << std::endl;
	if(inputImage.rows > 0 && maskImage.rows > 0){

		float *features = nscale::ObjFeatures::calcAvgFeatures(inputImage, maskImage);
		int nTotalFeatures = (nscale::ObjFeatures::N_INTENSITY_FEATURES+nscale::ObjFeatures::N_GRADIENT_FEATURES+nscale::ObjFeatures::N_CANNY_FEATURES);
		cv::Mat featuresMat(1, nTotalFeatures, CV_32FC1);
		float *featuresPtr = featuresMat.ptr<float>(0);
		std::cout << "AVG FEATURES" << std::endl;
		for(int i = 0; i < nTotalFeatures; i++){
			featuresPtr[i] = features[i];
			std::cout << featuresPtr[i]<<" ";
		}
		std::cout <<std::endl;
		free(features);

		this->features->setData(featuresMat);
	}else{
		std::cout << "Not Computing features" << std::endl;
	}
	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Feature Computation time elapsed: "<< t2-t1 << std::endl;
}

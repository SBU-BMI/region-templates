#include "RTCInterface.h"

void printComponentHello() {
	Worker *curWorker = Worker::getInstance();
	if(curWorker != NULL){
		curWorker->printHello();
		printf("So what?\n");

	}else{
		printf("Worker is NULL!!\n");
	}
}

cv::Mat* getDataRegion(string compId, string rtName, string dataRegionName, string drId, int timeStamp, int x, int y,
		int width, int height) {
	cv::Mat *ret = NULL;

	printf("Getting RT: %s DR: %s Time: %d CompId: %s (x=%d,y=%d,w=%d,h=%d)\n", rtName.c_str(), dataRegionName.c_str(), timeStamp, compId.c_str(), x, y, width, height);

	// Get pointer to the current Worker
	Worker* curWork = Worker::getInstance();

	if(curWork == NULL){
		std::cout << "WORKER IS NULLLLLL"<< std::endl;
		return ret;
	}

	RTPipelineComponentBase *rtComponent = dynamic_cast<RTPipelineComponentBase*>(curWork->retrieveActiveComponentRef(atoi(compId.c_str())));
	if(rtComponent != NULL){
#ifdef DEBUG
		std::cout << "Retrieved component references successfully!!!" << std::endl;
#endif
		RegionTemplate *rtData = rtComponent->getRegionTemplateInstance(rtName);
		if(rtData != NULL){
			DenseDataRegion2D *drData = dynamic_cast<DenseDataRegion2D*>(rtData->getDataRegion(dataRegionName, drId, timeStamp));
			//cv::Mat inputData = drData->getData();

			ret = drData->getData(x, y, width, height);
#ifdef DEBUG
			cv::imwrite("image-CPP-Interface.jpg", *ret);
			std::cout << "ret.channels: "<< ret->channels() << std::endl;
#endif
		}else{
			std::cout << "Could not find region template: "<< rtName << std::endl;
		}

	}else{
		std::cout << "RTComponent is NULL!!!"<< std::endl;
	}

	return ret;
}


#include "DistComp.h"

DistComp::DistComp() {
	neighborImage = new Mat();
	maskImage = new Mat();
	setComponentName("DistComp");
}

DistComp::~DistComp() {
	t2 = cciutils::ClockGetTime();
	delete neighborImage;
	delete maskImage;

	std::cout << "DistComp  took "<< t2-t1<< " ms" <<std::endl;
}

int DistComp::run()
{
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;

	std::string maskImageName = ((ArgumentString*)this->getArgument(0))->getArgValue();
	std::string tileSizeS = ((ArgumentString*)this->getArgument(1))->getArgValue();
	int tileSize = atoi(tileSizeS.c_str());

	Mat mask = cv::imread(maskImageName, -1);

	int zoomFactor = 4;
	if(zoomFactor > 1){
		Mat tempMask(mask.cols*zoomFactor ,mask.rows*zoomFactor, mask.type());
		for(int x = 0; x < zoomFactor; x++){
			for(int y = 0; y <zoomFactor; y++){
				Mat roiMask(tempMask, cv::Rect(mask.cols*x, mask.rows*y, mask.cols, mask.rows ));
				mask.copyTo(roiMask);
			}
		}
		mask = tempMask;
	}
	
	(*maskImage) = mask;

	if (!maskImage->data) {
		std::cout << "ERROR: invalid image: "<< maskImageName << std::endl;
		return false;
	}else{
		std::cout << "Yep. Image was successfully load: "<< maskImageName << std::endl;
	}

	int nTilesX=mask.cols/tileSize;
	int nTilesY=mask.rows/tileSize;

//	Mat *maskCopy = new Mat();
//	(*maskImage).copyTo((*maskCopy));
	this->neighborImage = new Mat(maskImage->size(), CV_32S);

	TaskFixProp *taskFixProp =  new TaskFixProp(neighborImage, maskImage, tileSize);
	t1 = cciutils::ClockGetTime();

	for(int tileY=0; tileY < nTilesY; tileY++){
		for(int tileX=0; tileX < nTilesX; tileX++){
			TaskTileProp *taskTileProp = new TaskTileProp(neighborImage, maskImage, tileX, tileY, tileSize, mask.cols);
			taskFixProp->addDependency(taskTileProp);
			this->executeTask(taskTileProp);
		}
	}

	this->executeTask(taskFixProp);


	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySeg() {
	return new DistComp();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("DistComp", &componentFactorySeg);



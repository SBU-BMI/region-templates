/*
 * FeatureExtraction.cpp
 *
 *  Created on: Feb 16, 2013
 *      Author: george
 */

#include "FeatureExtraction.h"
//#include "TaskSum.h"

FeatureExtraction::FeatureExtraction() {
	this->setComponentName("FeatureExtraction");
	this->addInputOutputDataRegion("tile", "BGR", RTPipelineComponentBase::INPUT);
	this->addInputOutputDataRegion("tile", "MASK", RTPipelineComponentBase::INPUT);
}

FeatureExtraction::~FeatureExtraction() {

}

int FeatureExtraction::run()
{
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	if(inputRt != NULL){
		DenseDataRegion2D *bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("BGR"));
		DenseDataRegion2D *mask = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("MASK"));
		if(bgr != NULL && mask != NULL){
			// Create processing task
			TaskFeatures * feaTask = new TaskFeatures(bgr, mask);

			this->executeTask(feaTask);
		}
	}else{
		std::cout << "\tDid not find RT named tile"<< std::endl;
	}

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactoryFE() {
	return new FeatureExtraction();
}

// register factory with the runtime system
bool registeredB = PipelineComponentBase::ComponentFactory::componentRegister("FeatureExtraction", &componentFactoryFE);



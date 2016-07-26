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
	this->addInputOutputDataRegion("tile", "mask", RTPipelineComponentBase::INPUT);
}

FeatureExtraction::~FeatureExtraction() {

}

int FeatureExtraction::run()
{
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");
	if(inputRt != NULL){
		std::cout << "\tfound RT named tile"<< std::endl;
		DenseDataRegion2D *bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("BGR"));
		DenseDataRegion2D *mask = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("mask"));

		if(bgr != NULL && mask != NULL){
			std::cout << "Feature computation: compId: "<< this->getId() <<std::endl;
#ifdef DEBUG
			std::cout << "bgr.rows: "<< bgr->getData().rows<< " maks.rows:"<< mask->getData().rows<< std::endl;
			std::cout << "FeatureExtraction:  MASK print" << std::endl;
			mask->print();
			std::cout << "END MASK print" << std::endl;
#endif
			// Create processing task
			TaskFeatures * feaTask = new TaskFeatures(bgr, mask);

			this->executeTask(feaTask);
		}
//		if(bgr !=NULL) inputRt->deleteDataRegion(bgr->getName(), bgr->getId(), bgr->getTimestamp(), bgr->getVersion());
//		if(mask !=NULL) inputRt->deleteDataRegion(mask->getName(), mask->getId(), mask->getTimestamp(), mask->getVersion());

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



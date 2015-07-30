/*
 * Segmentation.cpp
 *
 *  Created on: Feb 15, 2013
 *      Author: george
 */

#include "Segmentation.h"


Segmentation::Segmentation() {
	this->setComponentName("Segmentation");
	this->addInputOutputDataRegion("tile", "BGR", RTPipelineComponentBase::INPUT);
	this->addInputOutputDataRegion("tile", "mask", RTPipelineComponentBase::OUTPUT);
}

Segmentation::~Segmentation() {

}

int Segmentation::run()
{
	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	if(inputRt != NULL){
		DenseDataRegion2D *bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("BGR"));
		if(bgr != NULL){

			//std::cout <<  "nDataRegions: " << inputRt->getNumDataRegions() << std::endl;
#ifdef DEBUG
			std::cout << "Data Region is not null. rows: "<< bgr->getData().rows<< std::endl;
			bgr->print();
#endif
			// Create output data region
			DenseDataRegion2D *mask = new DenseDataRegion2D();
			mask->setName("mask");
			mask->setId(bgr->getId());

			inputRt->insertDataRegion(mask);

			// Create processing task
			TaskSegmentation * segTask = new TaskSegmentation(bgr, mask);

			this->executeTask(segTask);

		}else{
			std::cout << "Data Region is null"<< std::endl;
		}

	}else{
		std::cout << __FILE__ << ":" << __LINE__ <<" RT == NULL" << std::endl;
	}

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySeg() {
	return new Segmentation();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("Segmentation", &componentFactorySeg);



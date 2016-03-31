/*
 * Segmentation.cpp
 *
 *  Created on: Feb 15, 2013
 *      Author: george
 */

#include "Segmentation.h"


Segmentation::Segmentation() {
	this->setComponentName("Segmentation");
	this->addInputOutputDataRegion("tile", "RAW", RTPipelineComponentBase::INPUT);//_OUTPUT
	this->addInputOutputDataRegion("tile", "MASK", RTPipelineComponentBase::OUTPUT);//_OUTPUT
}

Segmentation::~Segmentation() {

}

int Segmentation::run()
{

	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	int parameterSegId = ((ArgumentInt*)this->getArgument(0))->getArgValue();
	float otsuRatio = (float)((ArgumentFloat*)this->getArgument(1))->getArgValue();
 	float curvatureWeight = (unsigned char)((ArgumentFloat*)this->getArgument(2))->getArgValue();
	float sizeThld = (unsigned char)((ArgumentFloat*)this->getArgument(3))->getArgValue();
	float sizeUpperThld = (unsigned char)((ArgumentFloat*)this->getArgument(4))->getArgValue();
	
	if(inputRt != NULL){
		DenseDataRegion2D *bgr = NULL;
		try{
			bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("RAW"));
			std::cout << "Segmentation. paramenterId: "<< parameterSegId<<std::endl;
		}catch(...){
			std::cout <<"ERROR SEGMENTATION " << std::endl;
			bgr=NULL;
		}
		if(bgr != NULL){
			std::cout << "Segmentation. BGR input id: "<< bgr->getId() << " paramenterId: "<< parameterSegId<<std::endl;
			// Create output data region
			DenseDataRegion2D *mask = new DenseDataRegion2D();
			mask->setName("MASK");
			mask->setId(bgr->getId());
			mask->setVersion(parameterSegId);

			inputRt->insertDataRegion(mask);
			std::cout <<  "nDataRegions: after:" << inputRt->getNumDataRegions() << std::endl;


			// Create processing task
			TaskSegmentation * segTask = new TaskSegmentation(bgr, mask, otsuRatio, curvatureWeight, sizeThld, sizeUpperThld );

			this->executeTask(segTask);

		}else{
			std::cout << __FILE__ << ":" << __LINE__ <<" DR == NULL" << std::endl;
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



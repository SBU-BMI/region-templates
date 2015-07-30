/*
 * Segmentation.cpp
 *
 *  Created on: Feb 15, 2013
 *      Author: george
 */

#include "Segmentation.h"


Segmentation::Segmentation() {
	this->setComponentName("Segmentation");
	this->addInputOutputDataRegion("tile", "BGR", RTPipelineComponentBase::INPUT);//_OUTPUT
	this->addInputOutputDataRegion("tile", "MASK", RTPipelineComponentBase::OUTPUT);
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
			std::cout << "Data Region is not null"<< std::endl;

			std::cout <<  "nDataRegions: " << inputRt->getNumDataRegions() << std::endl;
			// create id for the output data region
			FileUtils futils(".tif");
			std::string id = bgr->getId();
			std::string outputId = futils.replaceExt(id, ".bgr.pbm", ".mask.pbm");

			// Create output data region
			DenseDataRegion2D *mask = new DenseDataRegion2D();
			mask->setName("MASK");
			mask->setId(outputId);
			mask->setOutputType(DataSourceType::FILE_SYSTEM);

			inputRt->insertDataRegion(mask);
			std::cout <<  "nDataRegions: after:" << inputRt->getNumDataRegions() << std::endl;

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



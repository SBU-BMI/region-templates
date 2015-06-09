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
	this->addInputOutputDataRegion("tile", "FEATURES", RTPipelineComponentBase::OUTPUT);

}

FeatureExtraction::~FeatureExtraction() {

}

int FeatureExtraction::run()
{
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");
	std::string parameterId = ((ArgumentString*)this->getArgument(0))->getArgValue();

	if(inputRt != NULL){
		DenseDataRegion2D *bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("BGR", 0, atoi(parameterId.c_str())));
		DenseDataRegion2D *mask = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("MASK", 0, atoi(parameterId.c_str())));
		std::cout << "FeatureExtraction. BGR id: "<< bgr->getId() << " mask id: "<< mask->getId()<< " paramenterId: "<< parameterId<<std::endl;

		if(bgr != NULL && mask != NULL){
			// Create output data region
			DenseDataRegion2D *features = new DenseDataRegion2D();
			features->setName("FEATURES");
			features->setId(mask->getId());
			features->setVersion(atoi(parameterId.c_str()));
			features->setOutputExtension(DataRegion::XML);
			features->setOutputType(DataSourceType::FILE_SYSTEM);

			std::cout << "Before insertion" << std::endl;


			inputRt->insertDataRegion(features);
			std::cout << "After insertion" << std::endl;

			// Create processing task
			TaskFeatures * feaTask = new TaskFeatures(bgr, mask, features);

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



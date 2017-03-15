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
	this->addInputOutputDataRegion("tile", "features", RTPipelineComponentBase::OUTPUT);
}

FeatureExtraction::~FeatureExtraction() {

}

int FeatureExtraction::run()
{
    std::cout << "Entering FeatureExtraction::run()" << std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");
	if(inputRt != NULL){
		std::cout << "\tfound RT named tile"<< std::endl;
        inputRt->print();
		DenseDataRegion2D *bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("BGR"));
		DenseDataRegion2D *mask = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("mask"));

        //imwrite ("/lustre/atlas/proj-shared/csc143/lot/u24/test/DB-rt-adios/maskinfeature.png", mask->getData());


        std::cout << "bgr is " << bgr << " and mask is " << mask << std::endl;

		if(bgr != NULL && mask != NULL){
			std::cout << "Feature computation: compId: "<< this->getId() <<std::endl;
#ifdef DEBUG
			std::cout << "bgr.rows: "<< bgr->getData().rows<< " maks.rows:"<< mask->getData().rows<< std::endl;
			std::cout << "FeatureExtraction:  MASK print" << std::endl;
			mask->print();
			std::cout << "END MASK print" << std::endl;
#endif
			// Create output data region
			DataRegion2DUnaligned *features = new DataRegion2DUnaligned();
			features->setName("features");
			features->setId(bgr->getId());
			std::cout << "featuresId: "<< features->getId() << std::endl;
			// Because this is the application output, I want it to be written in the FS. Thus,
			// I will force the storage level 1 (defined in rtconf.xml) that must be a FS storage.
			features->setStorageLevel(1);
			inputRt->insertDataRegion(features);

            std::cout << "Added data region for feature output." << std::endl;
            inputRt->print();

			// Create processing task
			TaskFeatures * feaTask = new TaskFeatures(bgr, mask, features);

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



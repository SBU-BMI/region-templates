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

int Segmentation::run() {

    // Print name and id of the component instance
    std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() << std::endl;
    RegionTemplate *inputRt = this->getRegionTemplateInstance("tile");

    int parameterSegId = ((ArgumentInt *) this->getArgument(0))->getArgValue();
    float otsuRatio = (float) ((ArgumentFloat *) this->getArgument(1))->getArgValue();
    float curvatureWeight = ((ArgumentFloat *) this->getArgument(2))->getArgValue();
    float sizeThld = ((ArgumentFloat *) this->getArgument(3))->getArgValue();
    float sizeUpperThld = ((ArgumentFloat *) this->getArgument(4))->getArgValue();
    float mpp = (float) ((ArgumentFloat *) this->getArgument(5))->getArgValue();
    float mskernel = (float) ((ArgumentFloat *) this->getArgument(6))->getArgValue();
    int levelSetNumberOfIteration = ((ArgumentInt *) this->getArgument(7))->getArgValue();
    int declumpingType = ((ArgumentInt *) this->getArgument(8))->getArgValue();


    int *executionTime = (int *) malloc(2 * sizeof(int));
    executionTime[0] = 1; //Execution Time
    executionTime[1] = 1; //Image Number
    this->setResultData((char *) executionTime, 2 * sizeof(int));


    if (inputRt != NULL) {
        DenseDataRegion2D *bgr = NULL;
        try {
            bgr = dynamic_cast<DenseDataRegion2D *>(inputRt->getDataRegion("RAW"));
            std::cout << "Segmentation. paramenterId: " << parameterSegId << std::endl;
        } catch (...) {
            std::cout << "ERROR SEGMENTATION " << std::endl;
            bgr = NULL;
        }
        if (bgr != NULL) {
            std::cout << "Segmentation. BGR input id: " << bgr->getId() << " InputName: " << bgr->getInputFileName() <<
            " paramenterId: " << parameterSegId <<
            std::endl;
            // Create output data region
            DenseDataRegion2D *mask = new DenseDataRegion2D();
            mask->setName("MASK");
            mask->setId(bgr->getId());
            mask->setVersion(parameterSegId);
            mask->setInputFileName(bgr->getInputFileName());
            inputRt->insertDataRegion(mask);
            std::cout << "nDataRegions: after:" << inputRt->getNumDataRegions() << std::endl;


            // Create processing task
            TaskSegmentation *segTask = new TaskSegmentation(bgr, mask, otsuRatio, curvatureWeight, sizeThld,
                                                             sizeUpperThld, mpp, mskernel, levelSetNumberOfIteration,
                                                             declumpingType,
                                                             executionTime);

            this->executeTask(segTask);

        } else {
            std::cout << __FILE__ << ":" << __LINE__ << " DR == NULL" << std::endl;
        }

    } else {
        std::cout << __FILE__ << ":" << __LINE__ << " RT == NULL" << std::endl;
    }

    return 0;
}

// Create the component factory
PipelineComponentBase *componentFactorySeg() {
    return new Segmentation();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("Segmentation", &componentFactorySeg);



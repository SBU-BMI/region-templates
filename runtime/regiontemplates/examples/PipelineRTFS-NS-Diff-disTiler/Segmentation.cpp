/*
 * Segmentation.cpp
 *
 *  Created on: Feb 15, 2013
 *      Author: george
 */

#include "Segmentation.h"


Segmentation::Segmentation() {
    this->setComponentName("Segmentation");
}

Segmentation::~Segmentation() {

}

void Segmentation::setIo(std::string rtName, std::string ddrName) {
    this->addInputOutputDataRegion(rtName, ddrName, 
        RTPipelineComponentBase::INPUT);
    this->addInputOutputDataRegion(rtName, ddrName, 
        RTPipelineComponentBase::OUTPUT);
}

int Segmentation::run() {
    // Print name and id of the component instance
    std::cout << "Executing component: " << this->getComponentName() 
        << " instance id: " << this->getId() <<std::endl;

    std::string rtName = ((ArgumentString*)this->getArgument(0))->getArgValue();
    std::string ddrName = ((ArgumentString*)this->getArgument(1))->getArgValue();
    int blue = ((ArgumentInt*)this->getArgument(2))->getArgValue();
    int green = ((ArgumentInt*)this->getArgument(3))->getArgValue();
    int red = ((ArgumentInt*)this->getArgument(4))->getArgValue();
    float T1 = ((ArgumentFloat*)this->getArgument(5))->getArgValue();
    float T2 = ((ArgumentFloat*)this->getArgument(6))->getArgValue();
    int G1 = ((ArgumentInt*)this->getArgument(7))->getArgValue();
    int G2 = ((ArgumentInt*)this->getArgument(8))->getArgValue();
    int minSize = ((ArgumentInt*)this->getArgument(9))->getArgValue();
    int maxSize = ((ArgumentInt*)this->getArgument(10))->getArgValue();
    int minSizePl = ((ArgumentInt*)this->getArgument(11))->getArgValue();
    int minSizeSeg = ((ArgumentInt*)this->getArgument(12))->getArgValue();
    int maxSizeSeg = ((ArgumentInt*)this->getArgument(13))->getArgValue();
    int fillHolesConn = ((ArgumentInt*)this->getArgument(14))->getArgValue();
    int reconConn = ((ArgumentInt*)this->getArgument(15))->getArgValue();
    int watershedConn = ((ArgumentInt*)this->getArgument(16))->getArgValue();

    RegionTemplate * inputRt = this->getRegionTemplateInstance(rtName);

    if(inputRt != NULL) {
        DenseDataRegion2D *bgr = NULL;
        try{
            bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion(
                ddrName, NORM_DDR_OUTPUT_NAME, 0, NORM_DDR_OUTPUT_ID));
            std::cout << "Segmentation. paramenterId: "
                << NORM_DDR_OUTPUT_ID << std::endl;
        } catch (...) {
            std::cout << "ERROR SEGMENTATION " << std::endl;
            bgr=NULL;
        }
        if(bgr != NULL) {
            std::cout << "Segmentation. BGR input id: "<< bgr->getId() 
                << " paramenterId: "<< NORM_DDR_OUTPUT_ID << std::endl;
            // Create output data region
            DenseDataRegion2D *mask = new DenseDataRegion2D();
            mask->setName(ddrName);
            mask->setId(SEGM_DDR_OUTPUT_NAME);
            mask->setVersion(SEGM_DDR_OUTPUT_ID);

            inputRt->insertDataRegion(mask);
            std::cout <<  "nDataRegions: after:" 
                << inputRt->getNumDataRegions() << std::endl;

            // Create processing task
            TaskSegmentation * segTask = new TaskSegmentation(bgr, mask, 
                (unsigned char) blue, (unsigned char) green, 
                (unsigned char) red, (double) T1, (double) T2, 
                (unsigned char) G1, (unsigned char) G2, minSize, maxSize, 
                minSizePl, minSizeSeg, maxSizeSeg, fillHolesConn, 
                reconConn, watershedConn);

            this->executeTask(segTask);

        } else {
            std::cout << __FILE__ << ":" << __LINE__ 
                <<" DR == NULL" << std::endl;
        }

    } else {
        std::cout << __FILE__ << ":" << __LINE__ 
            << " RT == NULL" << std::endl;
    }

    return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySeg() {
    return new Segmentation();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister(
    "Segmentation", &componentFactorySeg);



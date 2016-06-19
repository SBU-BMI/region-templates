//
// Created by taveira on 6/4/16.
//

#include <regiontemplates/PolygonListDataRegion.h>
#include "PolygonExtractionComp.h"
#include "TaskPolygonExtraction.h"


PolygonExtractionComp::PolygonExtractionComp() {
    this->setComponentName("PolygonExtractionComp");
    this->addInputOutputDataRegion("tile", "RAW", RTPipelineComponentBase::INPUT);
    this->addInputOutputDataRegion("tile", "REF_MASK", RTPipelineComponentBase::INPUT);
    this->addInputOutputDataRegion("tile", "POLY_LIST", RTPipelineComponentBase::OUTPUT);
}

PolygonExtractionComp::~PolygonExtractionComp() {

}

int PolygonExtractionComp::run() {

    // Print name and id of the component instance
    RegionTemplate *inputRt = this->getRegionTemplateInstance("tile");
    int parameterId = ((ArgumentInt *) this->getArgument(0))->getArgValue();


    std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() << std::endl;

    if (inputRt != NULL) {
        DenseDataRegion2D *refmask = dynamic_cast<DenseDataRegion2D *>(inputRt->getDataRegion("REF_MASK"));
        if (refmask != NULL) {
            // Create output data region
            PolygonListDataRegion *polyList = new PolygonListDataRegion();
            // set outpu data region identifier
            polyList->setName("POLY_LIST");
            //polyList->setId(inputRt->getId());
            polyList->setId(refmask->getId());
            polyList->setVersion(parameterId);

            // insert data region into region template
            inputRt->insertDataRegion(polyList);

            // Create processing task
            TaskPolygonExtraction *polyTask = new TaskPolygonExtraction(refmask, polyList);

            this->executeTask(polyTask);

        } else {
            std::cout << __FILE__ << ":" << __LINE__ << " Data Region is == NULL" << std::endl;
        }

    } else {
        std::cout << __FILE__ << ":" << __LINE__ << " RT == NULL" << std::endl;
    }

    return 0;
}

// Create the component factory
PipelineComponentBase *componentFactoryPolyExtract() {
    return new PolygonExtractionComp();
}

// register factory with the runtime system
bool registeredP = PipelineComponentBase::ComponentFactory::componentRegister("PolygonExtractionComp",
                                                                              &componentFactoryPolyExtract);

#include <regiontemplates/comparativeanalysis/pixelcompare/PixelCompare.h>
#include <regiontemplates/comparativeanalysis/hadoopgis/predicates/DiceCoefficient.h>
#include <regiontemplates/comparativeanalysis/hadoopgis/predicates/JaccardIndex.h>
#include <regiontemplates/comparativeanalysis/hadoopgis/predicates/MaskIntersection.h>
#include <regiontemplates/PolygonListDataRegion.h>
#include "DiffMaskComp.h"
#include "TaskDiceDiffPolyList.h"

DiffMaskComp::DiffMaskComp() {
//	diffPercentage = 0.0;
    this->setComponentName("DiffMaskComp");
    this->addInputOutputDataRegion("tile", "MASK", RTPipelineComponentBase::INPUT);
    this->addInputOutputDataRegion("tile", "POLY_LIST", RTPipelineComponentBase::INPUT);

}

DiffMaskComp::~DiffMaskComp() {

}

int DiffMaskComp::run() {
    int parameterSegId = ((ArgumentInt *) this->getArgument(0))->getArgValue();

    RegionTemplate *inputRt = this->getRegionTemplateInstance("tile");
    PolygonListDataRegion *reference_poly_list = dynamic_cast<PolygonListDataRegion *>(inputRt->getDataRegion(
            "POLY_LIST", "", 0, 0));
//    DenseDataRegion2D *computed_mask = dynamic_cast<DenseDataRegion2D *>(inputRt->getDataRegion("MASK", "", 0, parameterSegId));
    float *diffPixels = (float *) malloc(3 * sizeof(float));
    diffPixels[0] = 0;
    diffPixels[1] = 0;
    diffPixels[2] = 0;
    this->setResultData((char *) diffPixels, 3 * sizeof(float));

    if (inputRt != NULL) {

        // Mask computed in segmentation using specific application parameter set
        DenseDataRegion2D *computed_mask = dynamic_cast<DenseDataRegion2D *>(inputRt->getDataRegion("MASK", "", 0,
                                                                                                    parameterSegId));
        // Mask used as a reference
        PolygonListDataRegion *reference_poly_list = dynamic_cast<PolygonListDataRegion *>(inputRt->getDataRegion(
                "POLY_LIST", "", 0, 0));

//        for (int i = 0; i < reference_poly_list->getData().size() ; ++i) {
//            long size = reference_poly_list->getData().at(i).size();
//            for (int j = 0; j < size; ++j) {
//                std::cout<<"############################################## " << reference_poly_list->getData().at(i).at(j) << std::endl;
//            }
//        }
        if (computed_mask != NULL && reference_poly_list != NULL) {
            // gambiarra
            diffPixels[0] = this->getId();
            // Create processing task
            TaskDiffMask *tDiffMask = new TaskDiceDiffPolyList(computed_mask, reference_poly_list, diffPixels);

            this->executeTask(tDiffMask);
        } else {
            std::cout << "DiffMaskComp: did not find data regions: " << std::endl;
            inputRt->print();

            //referenceRt->print();
        }
    } else {
        std::cout << "\tTASK diff mask: Did not find RT named tile" << std::endl;
    }

    return 0;
}

// Create the component factory
PipelineComponentBase *componentFactoryDF() {
    return new DiffMaskComp();
}

// register factory with the runtime system
bool registeredDF = PipelineComponentBase::ComponentFactory::componentRegister("DiffMaskComp", &componentFactoryDF);



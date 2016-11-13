#include <regiontemplates/comparativeanalysis/pixelcompare/PixelCompare.h>
#include <regiontemplates/comparativeanalysis/hadoopgis/knn/KnnBounded.h>
#include <regiontemplates/comparativeanalysis/hadoopgis/predicates/DiceCoefficient.h>
#include "DiffMaskComp.h"

DiffMaskComp::DiffMaskComp() {
//	diffPercentage = 0.0;
    //task = new PixelCompare();
    this->setComponentName("DiffMaskComp");
    this->addInputOutputDataRegion("tile", "MASK", RTPipelineComponentBase::INPUT);
}

DiffMaskComp::~DiffMaskComp() {

}

int DiffMaskComp::run() {
    int parameterSegId = ((ArgumentInt * )
    this->getArgument(0))->getArgValue();

    RegionTemplate *inputRt = this->getRegionTemplateInstance("tile");

    float *diffPixels = (float *) malloc(2 * sizeof(float));
    diffPixels[0] = 0;
    diffPixels[1] = 0;
    //(data,data size)
    this->setResultData((char *) diffPixels, 2 * sizeof(float));

    if (inputRt != NULL) {

        // Mask computed in segmentation using specific application parameter set
        DenseDataRegion2D *computed_mask = dynamic_cast<DenseDataRegion2D *>(inputRt->getDataRegion("MASK", "", 0,
                                                                                                    parameterSegId));
        // Mask used as a reference
        DenseDataRegion2D *reference_mask = dynamic_cast<DenseDataRegion2D *>(inputRt->getDataRegion("REF_MASK"));

        if (computed_mask != NULL && reference_mask != NULL) {
            // gambiarra
            diffPixels[0] = this->getId();
            cout << "------------------------------------------------- Calling Task:" << endl;

            std::vector<std::vector<cv::Point> > *list1;
            std::vector<std::vector<cv::Point> > *list2;
            Hadoopgis::getPolygonsFromLabeledMask(computed_mask->getData(), list1);
            Hadoopgis::getPolygonsFromLabeledMask(reference_mask->getData(), list2);
            float boundary = 10;
            TaskDiffMask *tDiffMask = new KnnBounded(list1, list2, diffPixels, 3, boundary);

//            TaskDiffMask *t2 = new DiceCoefficient();
//            t2->addDependency(tDiffMask);

            //TaskDiffMask *tDiffMask = new KnnUnbounded(computed_mask, reference_mask, diffPixels, 3);
            this->executeTask(tDiffMask);
            //this->executeTask(t2);

        } else {
            std::cout << "DiffMaskComp: did not find data regions: " << std::endl;
            inputRt->print();
        }
    } else {
        std::cout << "\tTASK diff mask: Did not find RT named tile" << std::endl;
    }

    return 0;
}

//void DiffMaskComp::setTask(TaskDiffMask* task){
//    this->task = task;
//}
// Create the component factory
PipelineComponentBase *componentFactoryDF() {
    return new DiffMaskComp();
}

// register factory with the runtime system
bool registeredDF = PipelineComponentBase::ComponentFactory::componentRegister("DiffMaskComp", &componentFactoryDF);



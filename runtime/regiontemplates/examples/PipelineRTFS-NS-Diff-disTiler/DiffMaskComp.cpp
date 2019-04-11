#include "DiffMaskComp.h"

DiffMaskComp::DiffMaskComp() {
    this->setComponentName("DiffMaskComp");
}

DiffMaskComp::~DiffMaskComp() {

}

void DiffMaskComp::setIo(std::string inRtName, std::string maskRtName, 
    std::string inDdrName, std::string maskDdrName) {
    
    this->addInputOutputDataRegion(inRtName, inDdrName, 
        RTPipelineComponentBase::INPUT);
    this->addInputOutputDataRegion(maskRtName, maskDdrName, 
        RTPipelineComponentBase::INPUT);
}

int DiffMaskComp::run() {
    std::string inRtName = 
        ((ArgumentString*)this->getArgument(0))->getArgValue();
    std::string maskRtName = 
        ((ArgumentString*)this->getArgument(1))->getArgValue();
    std::string inDdrName = 
        ((ArgumentString*)this->getArgument(2))->getArgValue();
    std::string maskDdrName = 
        ((ArgumentString*)this->getArgument(3))->getArgValue();

    RegionTemplate* inputRt = this->getRegionTemplateInstance(inRtName);
    RegionTemplate* maskRt = this->getRegionTemplateInstance(maskRtName);

    float *diffPixels = (float *) malloc(2 * sizeof(float));
    diffPixels[0] = 0;
    diffPixels[1] = 0;
    this->setResultData((char*)diffPixels, 2* sizeof(float));

    if(inputRt != NULL) {
        // Mask computed in segmentation using specific application parameter set
        DenseDataRegion2D *computed_mask = dynamic_cast<DenseDataRegion2D*>(
            inputRt->getDataRegion(inDdrName, SEGM_DDR_OUTPUT_NAME, 0, 
            SEGM_DDR_OUTPUT_ID));

        // Mask used as a reference
        DenseDataRegion2D *reference_mask = dynamic_cast<DenseDataRegion2D*>(
            maskRt->getDataRegion(maskDdrName));

        std::cout << "Looking for DR " << inDdrName << "." 
            << SEGM_DDR_OUTPUT_NAME << std::endl;
        if(reference_mask != NULL) {
            // gambiarra
            diffPixels[0] =  this->getId();
            TaskDiffMask *tDiffMask = new PixelCompare(
                computed_mask, reference_mask, diffPixels);
            this->executeTask(tDiffMask);
        } else {
            std::cout << "DiffMaskComp: did not find data regions: " << std::endl;
            // inputRt->print();
        }
    } else {
        std::cout << "\tTASK diff mask: Did not find RT named tile"<< std::endl;
    }

    return 0;
}

// Create the component factory
PipelineComponentBase* componentFactoryDF() {
    return new DiffMaskComp();
}

// register factory with the runtime system
bool registeredDF = PipelineComponentBase::ComponentFactory::componentRegister(
    "DiffMaskComp", &componentFactoryDF);



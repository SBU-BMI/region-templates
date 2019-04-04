#include "NormalizationComp.h"

NormalizationComp::NormalizationComp() {
    this->setComponentName("NormalizationComp");
}

NormalizationComp::~NormalizationComp() {

}

void NormalizationComp::setIo(std::string rtName, std::string ddrName) {
    this->addInputOutputDataRegion(rtName, ddrName, 
        RTPipelineComponentBase::INPUT);
    this->addInputOutputDataRegion(rtName, ddrName, 
        RTPipelineComponentBase::OUTPUT);
}

int NormalizationComp::run() {
    // Get the required RT and the args
    ArgumentFloatArray* args = ((ArgumentFloatArray*)this->getArgument(0));
    std::string rtName = ((ArgumentString*)this->getArgument(1))->getArgValue();
    std::string ddrName = ((ArgumentString*)this->getArgument(2))->getArgValue();
    RegionTemplate * inputRt = this->getRegionTemplateInstance(rtName);

    // Add the RT to the stage
    this->addInputOutputDataRegion(rtName, ddrName, 
        RTPipelineComponentBase::INPUT);
    this->addInputOutputDataRegion(rtName, ddrName, 
        RTPipelineComponentBase::OUTPUT);

    // Print name and id of the component instance
    std::cout << "Executing component: " << this->getComponentName() 
        << " instance id: " << this->getId() <<std::endl;

    if(inputRt != NULL) {
        DenseDataRegion2D *raw = dynamic_cast<DenseDataRegion2D*>(
            inputRt->getDataRegion(ddrName, INITIAL_DDR_INPUT_NAME));
        if(raw != NULL) {
            // Create output data region
            DenseDataRegion2D *bgr = new DenseDataRegion2D();

            // set output data region identifier
            bgr->setName(ddrName);
            bgr->setId(NORM_DDR_OUTPUT_NAME);
            bgr->setVersion(NORM_DDR_OUTPUT_ID);

            // insert data region into region template
            inputRt->insertDataRegion(bgr);

            float targetMean[3];
            for (int i = 0; i < 3; i++)
                targetMean[i] = args->getArgValue(i).getArgValue();

            // Create processing task
            TaskNormalization* normTask;
            normTask = new TaskNormalization(raw, bgr, targetMean);

            this->executeTask(normTask);

        } else {
            std::cout << __FILE__ << ":" << __LINE__ 
                <<" Data Region is == NULL, named: " << ddrName << std::endl;
        }

    } else {
        std::cout << __FILE__ << ":" << __LINE__ <<" RT == NULL" << std::endl;
    }

    return 0;
}

// Create the component factory
PipelineComponentBase* componentFactoryNorm() {
    return new NormalizationComp();
}

// register factory with the runtime system
bool registeredN = PipelineComponentBase::ComponentFactory::componentRegister(
    "NormalizationComp", &componentFactoryNorm);


#include "NormalizationComp.h"


NormalizationComp::NormalizationComp() {
	this->setComponentName("NormalizationComp");
	this->addInputOutputDataRegion("img", "initial", RTPipelineComponentBase::INPUT);
	this->addInputOutputDataRegion("img", "initial", RTPipelineComponentBase::OUTPUT);
}

NormalizationComp::~NormalizationComp() {

}

int NormalizationComp::run()
{

	// Print name and id of the component instance
	RegionTemplate * inputRt = this->getRegionTemplateInstance("img");
	int parameterId = ((ArgumentInt*)this->getArgument(0))->getArgValue();
	ArgumentFloatArray* args = ((ArgumentFloatArray*)this->getArgument(1));


	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;

	if(inputRt != NULL){
		DenseDataRegion2D *raw = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("initial"));
		if(raw != NULL){
			// Create output data region
			DenseDataRegion2D *bgr = new DenseDataRegion2D();
			// set outpu data region identifier
			bgr->setName("initial");
			bgr->setId("normalized");
			bgr->setVersion(1);

			// insert data region into region template
			inputRt->insertDataRegion(bgr);

			float targetMean[3];
			for(int i = 0; i < 3; i++) targetMean[i] = args->getArgValue(i).getArgValue();

			// Create processing task
			TaskNormalization * normTask = new TaskNormalization(raw, bgr, targetMean);

			this->executeTask(normTask);

		}else{
			std::cout << __FILE__ << ":" << __LINE__ <<" Data Region is == NULL" << std::endl;
		}

	}else{
		std::cout << __FILE__ << ":" << __LINE__ <<" RT == NULL" << std::endl;
	}

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactoryNorm() {
	return new NormalizationComp();
}

// register factory with the runtime system
bool registeredN = PipelineComponentBase::ComponentFactory::componentRegister("NormalizationComp", &componentFactoryNorm);

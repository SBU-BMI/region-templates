
#include "NormalizationComp.h"


NormalizationComp::NormalizationComp() {
	this->setComponentName("NormalizationComp");
	this->addInputOutputDataRegion("tile", "RAW", RTPipelineComponentBase::INPUT);
	this->addInputOutputDataRegion("tile", "BGR", RTPipelineComponentBase::OUTPUT);
}

NormalizationComp::~NormalizationComp() {

}

int NormalizationComp::run()
{

	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	if(inputRt != NULL){
		DenseDataRegion2D *raw = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("RAW"));
		if(raw != NULL){
			// create id for the output data region
			FileUtils futils(".tif");
			std::string id = raw->getId();
			std::string outputId = futils.replaceExt(id, ".tif", ".bgr.pbm");

			// Create output data region
			DenseDataRegion2D *bgr = new DenseDataRegion2D();
			bgr->setName("BGR");
			bgr->setId(outputId);
			bgr->setOutputType(DataSourceType::FILE_SYSTEM);

			inputRt->insertDataRegion(bgr);

			// Create processing task
			TaskNormalization * normTask = new TaskNormalization(raw, bgr);

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



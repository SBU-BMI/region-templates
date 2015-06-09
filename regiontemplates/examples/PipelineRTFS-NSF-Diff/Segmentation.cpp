/*
 * Segmentation.cpp
 *
 *  Created on: Feb 15, 2013
 *      Author: george
 */

#include "Segmentation.h"


Segmentation::Segmentation() {
	this->setComponentName("Segmentation");
	this->addInputOutputDataRegion("tile", "BGR", RTPipelineComponentBase::INPUT);//_OUTPUT
}

Segmentation::~Segmentation() {

}

int Segmentation::run()
{

	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	std::string parameterId = ((ArgumentString*)this->getArgument(0))->getArgValue();
	std::string drName = "MASK";
//	drName.append(parameterId);

	this->addInputOutputDataRegion("tile", drName, RTPipelineComponentBase::OUTPUT);

	if(inputRt != NULL){
		DenseDataRegion2D *bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("BGR", 0, atoi(parameterId.c_str())));
		std::cout << "Segmentation. BGR input id: "<< bgr->getId() << " paramenterId: "<< parameterId<<std::endl;
		if(bgr != NULL){
			// create id for the output data region
			FileUtils futils(".tif");
			std::string id = bgr->getId();
			//id.append(parameterId);
			std::string outputId = futils.replaceExt(id, ".bgr.pbm", ".mask.pbm");
			//std::string outputId = futils.replaceExt(id, ".bgr.pbm", std::string("-").append(parameterId));
			//outputId.append(".mask.pbm");

			// Create output data region
			DenseDataRegion2D *mask = new DenseDataRegion2D();
			mask->setName(drName);
			mask->setId(outputId);
			mask->setVersion(atoi(parameterId.c_str()));
			mask->setOutputType(DataSourceType::FILE_SYSTEM);

			inputRt->insertDataRegion(mask);
			std::cout <<  "nDataRegions: after:" << inputRt->getNumDataRegions() << std::endl;

			// Create processing task
			TaskSegmentation * segTask = new TaskSegmentation(bgr, mask);

			this->executeTask(segTask);

		}else{
			std::cout << __FILE__ << ":" << __LINE__ <<" DR == NULL" << std::endl;
		}

	}else{
		std::cout << __FILE__ << ":" << __LINE__ <<" RT == NULL" << std::endl;
	}


//	this->stageRegionTemplates();

/*	// Retrieve an argument value, and transform it to integer
	b = 1; //atoi (((ArgumentString*)this->getArgument(0))->getArgValue().c_str());

	// Create internal pipeline
	// Fist tasks without dependencies
	TaskSum* tA = new TaskSum(&aOut, &b, &c, "A.1");

	// Second task depending on tA
	TaskSum* tB = new TaskSum(&bOut, &aOut, &k1, "A.2");
	tB->addDependency(tA);

	// Third depends on tB
	TaskSum* tC = new TaskSum(&cOut, &bOut, &k2, "A.3");
	tC->addDependency(tB);

	// Dispatch tasks for execution with Resource Manager.
	this->executeTask(tA);
	this->executeTask(tB);
	this->executeTask(tC);*/

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySeg() {
	return new Segmentation();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("Segmentation", &componentFactorySeg);



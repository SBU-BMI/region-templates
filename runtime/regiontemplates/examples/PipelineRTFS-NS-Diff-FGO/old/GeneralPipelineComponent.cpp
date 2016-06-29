/*
 * GeneralPipelineComponent.cpp
 *
 *  Created on: May 23, 2016
 *      Author: willian
 */

#include "GeneralPipelineComponent.hpp"


GeneralPipelineComponent::GeneralPipelineComponent(std::string name) {
	this->setComponentName(name);
}

void GeneralPipelineComponent::addRtInput(std::string rtName, std::string regionName) {
	this->addInputOutputDataRegion(rtName, regionName, RTPipelineComponentBase::INPUT);//_INPUT
}

void GeneralPipelineComponent::addRtOutput(std::string rtName, std::string regionName) {
	this->addInputOutputDataRegion(rtName, regionName, RTPipelineComponentBase::OUTPUT);//_INPUT
}

GeneralPipelineComponent::~GeneralPipelineComponent() {}

int GeneralPipelineComponent::run()
{

	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	int parameterId = ((ArgumentInt*)this->getArgument(0))->getArgValue();
	int parameterSegId = ((ArgumentInt*)this->getArgument(1))->getArgValue();
	unsigned char blue = (unsigned char)((ArgumentInt*)this->getArgument(2))->getArgValue();
	unsigned char green = (unsigned char)((ArgumentInt*)this->getArgument(3))->getArgValue();
	unsigned char red = (unsigned char)((ArgumentInt*)this->getArgument(4))->getArgValue();
	double T1 = (double)((ArgumentFloat*)this->getArgument(5))->getArgValue();
	double T2 = (double)((ArgumentFloat*)this->getArgument(6))->getArgValue();
	unsigned char G1 = (unsigned char)((ArgumentInt*)this->getArgument(7))->getArgValue();
	unsigned char G2 = (unsigned char)((ArgumentInt*)this->getArgument(8))->getArgValue();
	int minSize = ((ArgumentInt*)this->getArgument(9))->getArgValue();
	int maxSize = ((ArgumentInt*)this->getArgument(10))->getArgValue();
	int minSizePl = ((ArgumentInt*)this->getArgument(11))->getArgValue();
	int minSizeSeg = ((ArgumentInt*)this->getArgument(12))->getArgValue();
	int maxSizeSeg= ((ArgumentInt*)this->getArgument(13))->getArgValue();
	int fillHolesConnectivity= ((ArgumentInt*)this->getArgument(14))->getArgValue();
	int reconConnectivity= ((ArgumentInt*)this->getArgument(15))->getArgValue();
	int watershedConnectivity= ((ArgumentInt*)this->getArgument(16))->getArgValue();

	if(inputRt != NULL){
		DenseDataRegion2D *bgr = NULL;
		try{
			bgr = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion("BGR", "", 0, parameterId));
			std::cout << "GeneralPipelineComponent. paramenterId: "<< parameterId<<std::endl;
		}catch(...){
			std::cout <<"ERROR GeneralPipelineComponent " << std::endl;
			bgr=NULL;
		}
		if(bgr != NULL){
			std::cout << "GeneralPipelineComponent. BGR input id: "<< bgr->getId() << " paramenterId: "<< parameterId<<std::endl;
			// Create output data region
			DenseDataRegion2D *mask = new DenseDataRegion2D();
			mask->setName("MASK");
			mask->setId(bgr->getId());
			mask->setVersion(parameterSegId);

			inputRt->insertDataRegion(mask);
			std::cout <<  "nDataRegions: after:" << inputRt->getNumDataRegions() << std::endl;


			// Create processing task
			TaskGeneralPipelineComponent * segTask = new TaskGeneralPipelineComponent(bgr, mask, blue, green, red, T1, T2, G1, G2, minSize, maxSize, minSizePl, minSizeSeg, maxSizeSeg, fillHolesConnectivity, reconConnectivity, watershedConnectivity);

			this->executeTask(segTask);

		}else{
			std::cout << __FILE__ << ":" << __LINE__ <<" DR == NULL" << std::endl;
		}

	}else{
		std::cout << __FILE__ << ":" << __LINE__ <<" RT == NULL" << std::endl;
	}

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySeg() {
	return new GeneralPipelineComponent();
}

// register factory with the runtime system
bool registered = PipelineComponentBase::ComponentFactory::componentRegister("GeneralPipelineComponent", &componentFactorySeg);



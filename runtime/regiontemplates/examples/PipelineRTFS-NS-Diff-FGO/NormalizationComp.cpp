/*
 * NormalizationComp.cpp
 *
 *  GENERATED CODE
 *  DO NOT CHANGE IT MANUALLY!!!!!
 */

#include "NormalizationComp.hpp"

#include <string>
#include <sstream>

/**************************************************************************************/
/**************************** PipelineComponent functions *****************************/
/**************************************************************************************/

NormalizationComp::NormalizationComp() {
	this->setComponentName("NormalizationComp");
		this->addInputOutputDataRegion("tile", "normalizedImg", RTPipelineComponentBase::OUTPUT);

}

NormalizationComp::~NormalizationComp() {}

int NormalizationComp::run() {

	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	std::string inputImage_name;
	float* targetMean;
	float* targetStd;

	int set_cout = 0;
	for(int i=0; i<this->getArgumentsSize(); i++){
		if (this->getArgument(i)->getName().compare("inputImage") == 0) {
			inputImage_name = (std::string)((ArgumentString*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("targetMean") == 0) {
			targetMean = (float*)((ArgumentFloatArray*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("targetStd") == 0) {
			targetStd = (float*)((ArgumentFloatArray*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

	}

	if (set_cout < this->getArgumentsSize())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on NormalizationComp" << std::endl;

	this->addInputOutputDataRegion("tile", inputImage_name, RTPipelineComponentBase::INPUT);

	if(inputRt != NULL){
		DenseDataRegion2D *inputImage = NULL;

		try{
			inputImage = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion(inputImage_name, "", 0, dr_id));

			std::cout << "NormalizationComp. paramenterId: "<< dr_id <<std::endl;
		}catch(...){
			std::cout <<"ERROR NormalizationComp " << std::endl;
		}

		ostringstream conv;
		conv << dr_id;
		std::string dr_id_s(conv.str());
		char* dr_id_c = new char[dr_id_s.length()];
		for (int i=0; i<dr_id_s.length(); i++)
			dr_id_c[i] = dr_id_s[i];
					
		// Create output data region
		DenseDataRegion2D *normalizedImg = new DenseDataRegion2D();
		normalizedImg->setName("normalizedImg");
		normalizedImg->setId(dr_id_c);
		normalizedImg->setVersion(0);
		inputRt->insertDataRegion(normalizedImg);

		std::cout <<  "nDataRegions: after:" << inputRt->getNumDataRegions() << std::endl;

		// Create processing task
		TaskNormalizationComp * task = new TaskNormalizationComp(inputImage, normalizedImg, targetMean, targetStd);

		this->executeTask(task);


	}else{
		std::cout << __FILE__ << ":" << __LINE__ <<" RT == NULL" << std::endl;
	}

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactoryNormalizationComp() {
	return new NormalizationComp();
}

// register factory with the runtime system
bool registeredNormalizationComp = PipelineComponentBase::ComponentFactory::componentRegister("NormalizationComp", &componentFactoryNormalizationComp);


/**************************************************************************************/
/*********************************** Task functions ***********************************/
/**************************************************************************************/

TaskNormalizationComp::TaskNormalizationComp(DenseDataRegion2D* inputImage_temp, DenseDataRegion2D* normalizedImg_temp, float* targetMean, float* targetStd) {
	
	this->inputImage_temp = inputImage_temp;
	this->normalizedImg_temp = normalizedImg_temp;
	this->targetMean = targetMean;
	this->targetStd = targetStd;

	
}

TaskNormalizationComp::~TaskNormalizationComp() {
	if(inputImage_temp != NULL) delete inputImage_temp;

}

bool TaskNormalizationComp::run(int procType, int tid) {
	
	cv::Mat inputImage = this->inputImage_temp->getData();

	cv::Mat normalizedImg = this->normalizedImg_temp->getData();

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskNormalizationComp executing." << std::endl;	

	normalizedImg = ::nscale::Normalization::normalization(inputImage, targetMean, targetStd);
	
	this->normalizedImg_temp->setData(normalizedImg);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task NormalizationComp time elapsed: "<< t2-t1 << std::endl;
}

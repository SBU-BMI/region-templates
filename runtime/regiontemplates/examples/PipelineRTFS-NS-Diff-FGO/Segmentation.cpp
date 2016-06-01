/*
 * Segmentation.cpp
 *
 *  GENERATED CODE
 *  DO NOT CHANGE IT MANUALLY!!!!!
 */

#include "Segmentation.hpp"

#include <string>
#include <sstream>

/**************************************************************************************/
/**************************** PipelineComponent functions *****************************/
/**************************************************************************************/

Segmentation::Segmentation() {
	this->setComponentName("Segmentation");
		this->addInputOutputDataRegion("tile", "outMask", RTPipelineComponentBase::OUTPUT);

}

Segmentation::~Segmentation() {}

int Segmentation::run() {

	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	std::string inputImage_name;
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	double T1;
	double T2;
	unsigned char G1;
	int minSize;
	int maxSize;
	unsigned char G2;
	int minSizePl;
	int minSizeSeg;
	int maxSizeSeg;
	int fillHolesConnectivity;
	int reconConnectivity;
	int watershedConnectivity;

	int set_cout = 0;
	for(int i=0; i<this->getArgumentsSize(); i++){
		if (this->getArgument(i)->getName().compare("inputImage") == 0) {
			inputImage_name = (std::string)((ArgumentString*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("blue") == 0) {
			blue = (unsigned char)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("green") == 0) {
			green = (unsigned char)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("red") == 0) {
			red = (unsigned char)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("T1") == 0) {
			T1 = (double)((ArgumentFloat*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("T2") == 0) {
			T2 = (double)((ArgumentFloat*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("G1") == 0) {
			G1 = (unsigned char)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("minSize") == 0) {
			minSize = (int)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("maxSize") == 0) {
			maxSize = (int)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("G2") == 0) {
			G2 = (unsigned char)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("minSizePl") == 0) {
			minSizePl = (int)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("minSizeSeg") == 0) {
			minSizeSeg = (int)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("maxSizeSeg") == 0) {
			maxSizeSeg = (int)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("fillHolesConnectivity") == 0) {
			fillHolesConnectivity = (int)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("reconConnectivity") == 0) {
			reconConnectivity = (int)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("watershedConnectivity") == 0) {
			watershedConnectivity = (int)((ArgumentInt*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

	}

	if (set_cout < this->getArgumentsSize())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;

	this->addInputOutputDataRegion("tile", inputImage_name, RTPipelineComponentBase::INPUT);

	if(inputRt != NULL){
		DenseDataRegion2D *inputImage = NULL;

		try{
			inputImage = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion(inputImage_name, "", 0, dr_id));

			std::cout << "Segmentation. paramenterId: "<< dr_id <<std::endl;
		}catch(...){
			std::cout <<"ERROR Segmentation " << std::endl;
		}

		ostringstream conv;
		conv << dr_id;
		std::string dr_id_s(conv.str());
		char* dr_id_c = new char[dr_id_s.length()];
		for (int i=0; i<dr_id_s.length(); i++)
			dr_id_c[i] = dr_id_s[i];
					
		// Create output data region
		DenseDataRegion2D *outMask = new DenseDataRegion2D();
		outMask->setName("outMask");
		outMask->setId(dr_id_c);
		outMask->setVersion(0);
		inputRt->insertDataRegion(outMask);

		std::cout <<  "nDataRegions: after:" << inputRt->getNumDataRegions() << std::endl;

		// Create processing task
		TaskSegmentation * task = new TaskSegmentation(inputImage, outMask, blue, green, red, T1, T2, G1, minSize, maxSize, G2, minSizePl, minSizeSeg, maxSizeSeg, fillHolesConnectivity, reconConnectivity, watershedConnectivity);

		this->executeTask(task);


	}else{
		std::cout << __FILE__ << ":" << __LINE__ <<" RT == NULL" << std::endl;
	}

	return 0;
}

// Create the component factory
PipelineComponentBase* componentFactorySegmentation() {
	return new Segmentation();
}

// register factory with the runtime system
bool registeredSegmentation = PipelineComponentBase::ComponentFactory::componentRegister("Segmentation", &componentFactorySegmentation);


/**************************************************************************************/
/*********************************** Task functions ***********************************/
/**************************************************************************************/

TaskSegmentation::TaskSegmentation(DenseDataRegion2D* inputImage_temp, DenseDataRegion2D* outMask_temp, unsigned char blue, unsigned char green, unsigned char red, double T1, double T2, unsigned char G1, int minSize, int maxSize, unsigned char G2, int minSizePl, int minSizeSeg, int maxSizeSeg, int fillHolesConnectivity, int reconConnectivity, int watershedConnectivity) {
	
	this->inputImage_temp = inputImage_temp;
	this->outMask_temp = outMask_temp;
	this->blue = blue;
	this->green = green;
	this->red = red;
	this->T1 = T1;
	this->T2 = T2;
	this->G1 = G1;
	this->minSize = minSize;
	this->maxSize = maxSize;
	this->G2 = G2;
	this->minSizePl = minSizePl;
	this->minSizeSeg = minSizeSeg;
	this->maxSizeSeg = maxSizeSeg;
	this->fillHolesConnectivity = fillHolesConnectivity;
	this->reconConnectivity = reconConnectivity;
	this->watershedConnectivity = watershedConnectivity;

	
}

TaskSegmentation::~TaskSegmentation() {
	if(inputImage_temp != NULL) delete inputImage_temp;

}

bool TaskSegmentation::run(int procType, int tid) {
	
	cv::Mat inputImage = this->inputImage_temp->getData();

	cv::Mat outMask = this->outMask_temp->getData();

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentation executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNuclei(inputImage, outMask, blue, green, red, T1, T2, G1, minSize, maxSize, G2, minSizePl, minSizeSeg, maxSizeSeg, fillHolesConnectivity, reconConnectivity, watershedConnectivity);
	
	this->outMask_temp->setData(outMask);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

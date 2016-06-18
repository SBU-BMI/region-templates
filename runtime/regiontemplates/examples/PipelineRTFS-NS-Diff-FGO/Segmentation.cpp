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
}

Segmentation::~Segmentation() {}

int Segmentation::run() {

	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	std::string normalized_rt_name;
	std::string segmented_rt_name;
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
		if (this->getArgument(i)->getName().compare("normalized_rt") == 0) {
			normalized_rt_name = (std::string)((ArgumentString*)this->getArgument(i))->getArgValue();
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("segmented_rt") == 0) {
			segmented_rt_name = (std::string)((ArgumentString*)this->getArgument(i))->getArgValue();
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

	this->addInputOutputDataRegion("tile", normalized_rt_name, RTPipelineComponentBase::INPUT);

	this->addInputOutputDataRegion("tile", segmented_rt_name, RTPipelineComponentBase::OUTPUT);


	if(inputRt != NULL){
		DenseDataRegion2D *normalized_rt = NULL;

		DenseDataRegion2D *segmented_rt = NULL;

		try{
			normalized_rt = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion(normalized_rt_name, "", 0, workflow_id));

			segmented_rt = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion(segmented_rt_name, "", 0, workflow_id));

			std::cout << "Segmentation. paramenterId: "<< workflow_id <<std::endl;
		}catch(...){
			std::cout <<"ERROR Segmentation " << std::endl;
		}

		// Create processing task
		TaskSegmentation * task = new TaskSegmentation(normalized_rt, segmented_rt, blue, green, red, T1, T2, G1, minSize, maxSize, G2, minSizePl, minSizeSeg, maxSizeSeg, fillHolesConnectivity, reconConnectivity, watershedConnectivity);

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

TaskSegmentation::TaskSegmentation(DenseDataRegion2D* normalized_rt_temp, DenseDataRegion2D* segmented_rt_temp, unsigned char blue, unsigned char green, unsigned char red, double T1, double T2, unsigned char G1, int minSize, int maxSize, unsigned char G2, int minSizePl, int minSizeSeg, int maxSizeSeg, int fillHolesConnectivity, int reconConnectivity, int watershedConnectivity) {
	
	this->normalized_rt_temp = normalized_rt_temp;
	this->segmented_rt_temp = segmented_rt_temp;
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
	if(normalized_rt_temp != NULL) delete normalized_rt_temp;

}

bool TaskSegmentation::run(int procType, int tid) {
	
	cv::Mat normalized_rt = this->normalized_rt_temp->getData();

	cv::Mat segmented_rt = this->segmented_rt_temp->getData();

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentation executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNuclei(normalized_rt, segmented_rt, blue, green, red, T1, T2, G1, minSize, maxSize, G2, minSizePl, minSizeSeg, maxSizeSeg, fillHolesConnectivity, reconConnectivity, watershedConnectivity);
	
	this->segmented_rt_temp->setData(segmented_rt);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

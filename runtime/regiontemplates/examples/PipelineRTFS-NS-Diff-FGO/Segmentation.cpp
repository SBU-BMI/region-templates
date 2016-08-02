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

	ArgumentRT* normalized_rt_arg;
	ArgumentRT* segmented_rt_arg;
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
			normalized_rt_arg = (ArgumentRT*)this->getArgument(i);
			set_cout++;
		}

		if (this->getArgument(i)->getName().compare("segmented_rt") == 0) {
			segmented_rt_arg = (ArgumentRT*)this->getArgument(i);
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

	this->addInputOutputDataRegion("tile", normalized_rt_arg->getName(), RTPipelineComponentBase::INPUT);

	this->addInputOutputDataRegion("tile", segmented_rt_arg->getName(), RTPipelineComponentBase::OUTPUT);


	if(inputRt != NULL){
		DenseDataRegion2D *normalized_rt = NULL;

		DenseDataRegion2D *segmented_rt = NULL;

		try{
			normalized_rt = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion(
				normalized_rt_arg->getName(), std::to_string(normalized_rt_arg->getId()), 0, normalized_rt_arg->getId()));

			segmented_rt = new DenseDataRegion2D();
			segmented_rt->setName(segmented_rt_arg->getName());
			segmented_rt->setId(std::to_string(segmented_rt_arg->getId()));
			segmented_rt->setVersion(segmented_rt_arg->getId());
			inputRt->insertDataRegion(segmented_rt);

			std::cout << "Segmentation. paramenterId: "<< workflow_id <<std::endl;
		}catch(...){
			std::cout <<"ERROR Segmentation " << std::endl;
		}

		// Create processing task
		TaskSegmentation * task = new TaskSegmentation(normalized_rt, segmented_rt, blue, 
			green, red, T1, T2, G1, minSize, maxSize, G2, minSizePl, minSizeSeg, maxSizeSeg, 
			fillHolesConnectivity, reconConnectivity, watershedConnectivity);

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

TaskSegmentation::TaskSegmentation(DenseDataRegion2D* normalized_rt_temp, 
	DenseDataRegion2D* segmented_rt_temp, unsigned char blue, unsigned char green, unsigned char red, 
	double T1, double T2, unsigned char G1, int minSize, int maxSize, unsigned char G2, 
	int minSizePl, int minSizeSeg, int maxSizeSeg, int fillHolesConnectivity, int reconConnectivity, int watershedConnectivity) {
	
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

	cv::Mat segmented_rt;

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "TaskSegmentation executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNuclei(normalized_rt, segmented_rt, blue, green, 
		red, T1, T2, G1, minSize, maxSize, G2, minSizePl, minSizeSeg, maxSizeSeg, fillHolesConnectivity, 
		reconConnectivity, watershedConnectivity);
	
	this->segmented_rt_temp->setData(segmented_rt);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "Task Segmentation time elapsed: "<< t2-t1 << std::endl;
}

bool TaskSegmentation::reusable(ReusableTask* rt) {
	TaskSegmentation* t = (TaskSegmentation*)(rt);
	if (this->normalized_rt_temp->getName() == t->normalized_rt_temp->getName() &&
		this->normalized_rt_temp->getId() == t->normalized_rt_temp->getId() &&
		this->normalized_rt_temp->getVersion() == t->normalized_rt_temp->getVersion() &&
		this->segmented_rt_temp->getName() == t->segmented_rt_temp->getName() &&
		this->segmented_rt_temp->getId() == t->segmented_rt_temp->getId() &&
		this->segmented_rt_temp->getVersion() == t->segmented_rt_temp->getVersion() &&
		this->blue == t->blue &&
		this->green == t->green &&
		this->red == t->red &&
		this->T1 == t->T1 &&
		this->T2 == t->T2 &&
		this->G1 == t->G1 &&
		this->minSize == t->minSize &&
		this->maxSize == t->maxSize &&
		this->G2 == t->G2 &&
		this->minSizePl == t->minSizePl &&
		this->minSizeSeg == t->minSizeSeg &&
		this->maxSizeSeg == t->maxSizeSeg &&
		this->fillHolesConnectivity == t->fillHolesConnectivity &&
		this->reconConnectivity == t->reconConnectivity &&
		this->watershedConnectivity == t->watershedConnectivity) {

		return true;
	} else {
		return false;
	}
}

int TaskSegmentation::size() {
	return sizeof(unsigned char) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(float) + 
		sizeof(float) + sizeof(unsigned char) + sizeof(int) + sizeof(int) + sizeof(unsigned char) + 
		sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int);
}

int TaskSegmentation::serialize(char *buff) {
	int serialized_bytes = 0;

	// copy field blue
	memcpy(buff+serialized_bytes, &blue, sizeof(unsigned char));
		serialized_bytes+=sizeof(unsigned char);

	// copy field green
	memcpy(buff+serialized_bytes, &green, sizeof(unsigned char));
		serialized_bytes+=sizeof(unsigned char);

	// copy field red
	memcpy(buff+serialized_bytes, &red, sizeof(unsigned char));
		serialized_bytes+=sizeof(unsigned char);
		
	// copy field T1
	memcpy(buff+serialized_bytes, &T1, sizeof(float));
		serialized_bytes+=sizeof(float);
		
	// copy field T2
	memcpy(buff+serialized_bytes, &T2, sizeof(float));
		serialized_bytes+=sizeof(float);
		
	// copy field G1
	memcpy(buff+serialized_bytes, &G1, sizeof(unsigned char));
		serialized_bytes+=sizeof(unsigned char);
		
	// copy field minSize
	memcpy(buff+serialized_bytes, &minSize, sizeof(int));
		serialized_bytes+=sizeof(int);

	// copy field maxSize
	memcpy(buff+serialized_bytes, &maxSize, sizeof(int));
		serialized_bytes+=sizeof(int);

	// copy field G2
	memcpy(buff+serialized_bytes, &G2, sizeof(unsigned char));
		serialized_bytes+=sizeof(unsigned char);
		
	// copy field minSizePl
	memcpy(buff+serialized_bytes, &minSizePl, sizeof(int));
		serialized_bytes+=sizeof(int);

	// copy field minSizeSeg
	memcpy(buff+serialized_bytes, &minSizeSeg, sizeof(int));
		serialized_bytes+=sizeof(int);

	// copy field maxSizeSeg
	memcpy(buff+serialized_bytes, &maxSizeSeg, sizeof(int));
		serialized_bytes+=sizeof(int);

	// copy field fillHolesConnectivity
	memcpy(buff+serialized_bytes, &fillHolesConnectivity, sizeof(int));
		serialized_bytes+=sizeof(int);

	// copy field reconConnectivity
	memcpy(buff+serialized_bytes, &reconConnectivity, sizeof(int));
		serialized_bytes+=sizeof(int);

	// copy field watershedConnectivity
	memcpy(buff+serialized_bytes, &watershedConnectivity, sizeof(int));
		serialized_bytes+=sizeof(int);

	return serialized_bytes;
}

int TaskSegmentation::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract field blue
	this->blue = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field red
	this->red = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field green
	this->green = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field T1
	this->T1 = ((float*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(float);

	// extract field T2
	this->T2 = ((float*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(float);

	// extract field G1
	this->G1 = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field G2
	this->G2 = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field minSize
	this->minSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field maxSize
	this->maxSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field minSizePl
	this->minSizePl = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field minSizeSeg
	this->minSizeSeg = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field maxSizeSeg
	this->maxSizeSeg = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field fillHolesConnectivity
	this->fillHolesConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field reconConnectivity
	this->reconConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field watershedConnectivity
	this->watershedConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	return deserialized_bytes;
}

ReusableTask* TaskSegmentation::clone() {
	ReusableTask* retValue = new TaskSegmentation();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;

	return retValue;
}

// Create the task factory
ReusableTask* taskFactorySegmentation1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new TaskSegmentation(args, inputRt);
}

// Create the task factory
ReusableTask* taskFactorySegmentation2() {
	return new TaskSegmentation();
} 

// register factory with the runtime system
bool registeredSegmentationTask = ReusableTask::ReusableTaskFactory::taskRegister("TaskSegmentation", 
	&taskFactorySegmentation1, &taskFactorySegmentation2);
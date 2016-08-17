/*
 * Segmentation.cpp
 *
 *  GENERATED CODE
 *  DO NOT CHANGE IT MANUALLY!!!!!
 */

#include "Segmentation.hpp"

#include <string>
#include <sstream>
#include <typeinfo>

/**************************************************************************************/
/**************************** PipelineComponent functions *****************************/
/**************************************************************************************/

Segmentation::Segmentation() {
	this->setComponentName("Segmentation");

	// generate task descriptors
	list<ArgumentBase*> segmentation_task1_args;
	list<ArgumentBase*> segmentation_task2_args;
	list<ArgumentBase*> segmentation_task3_args;

	ArgumentRT* normalized_rt = new ArgumentRT();
	normalized_rt->setName("normalized_rt");
	segmentation_task1_args.emplace_back(normalized_rt);
	ArgumentInt* blue = new ArgumentInt();
	blue->setName("blue");
	segmentation_task1_args.emplace_back(blue);
	ArgumentInt* green = new ArgumentInt();
	green->setName("green");
	segmentation_task1_args.emplace_back(green);
	ArgumentInt* red = new ArgumentInt();
	red->setName("red");
	segmentation_task1_args.emplace_back(red);
	ArgumentFloat* T1 = new ArgumentFloat();
	T1->setName("T1");
	segmentation_task1_args.emplace_back(T1);
	ArgumentFloat* T2 = new ArgumentFloat();
	T2->setName("T2");
	segmentation_task1_args.emplace_back(T2);
	ArgumentInt* G1 = new ArgumentInt();
	G1->setName("G1");
	segmentation_task1_args.emplace_back(G1);
	ArgumentInt* minSize = new ArgumentInt();
	minSize->setName("minSize");
	segmentation_task1_args.emplace_back(minSize);
	ArgumentInt* maxSize = new ArgumentInt();
	maxSize->setName("maxSize");
	segmentation_task1_args.emplace_back(maxSize);
	ArgumentInt* G2 = new ArgumentInt();
	G2->setName("G2");
	segmentation_task1_args.emplace_back(G2);
	ArgumentInt* fillHolesConnectivity = new ArgumentInt();
	fillHolesConnectivity->setName("fillHolesConnectivity");
	segmentation_task1_args.emplace_back(fillHolesConnectivity);
	ArgumentInt* reconConnectivity = new ArgumentInt();
	reconConnectivity->setName("reconConnectivity");
	segmentation_task1_args.emplace_back(reconConnectivity);
	this->tasksDesc["Task1Segmentation"] = segmentation_task1_args;
	
	segmentation_task2_args.emplace_back(normalized_rt);
	ArgumentInt* minSizePl = new ArgumentInt();
	minSizePl->setName("minSizePl");
	segmentation_task2_args.emplace_back(minSizePl);
	ArgumentInt* watershedConnectivity = new ArgumentInt();
	watershedConnectivity->setName("watershedConnectivity");
	segmentation_task2_args.emplace_back(watershedConnectivity);
	this->tasksDesc["Task2Segmentation"] = segmentation_task2_args;
	
	ArgumentInt* minSizeSeg = new ArgumentInt();
	minSizeSeg->setName("minSizeSeg");
	segmentation_task3_args.emplace_back(minSizeSeg);
	ArgumentInt* maxSizeSeg = new ArgumentInt();
	maxSizeSeg->setName("maxSizeSeg");
	segmentation_task3_args.emplace_back(maxSizeSeg);
	segmentation_task3_args.emplace_back(fillHolesConnectivity);
	ArgumentRT* segmented_rt = new ArgumentRT();
	segmented_rt->setName("segmented_rt");
	segmentation_task3_args.emplace_back(segmented_rt);
	this->tasksDesc["Task3Segmentation"] = segmentation_task3_args;
}

Segmentation::~Segmentation() {}

int Segmentation::run() {

	// Print name and id of the component instance
	std::cout << "Executing component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	this->addInputOutputDataRegion("tile", "normalized_rt", RTPipelineComponentBase::INPUT);
	this->addInputOutputDataRegion("tile", "segmented_rt", RTPipelineComponentBase::OUTPUT);

	map<int, ReusableTask*> prev_map;
	for (list<ReusableTask*>::reverse_iterator task=tasks.rbegin(); task!=tasks.rend(); task++) {
		cout << "[Segmentation] sending task " << (*task)->getId() << endl;
		// generate a task copy and update the DR, getting the actual data
		ReusableTask* t = (*task)->clone();
		t->updateDR(inputRt);

		// solve dependency if it isn't the first task
		if (t->parentTask != -1) {
			t->addDependency(prev_map[t->parentTask]);
			t->resolveDependencies(prev_map[t->parentTask]);
		}

		// send task to be executed
		this->executeTask(t);

		// add this task to parent list for future dependency resolution
		prev_map[t->getId()] = t;
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

Task1Segmentation::Task1Segmentation(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	int set_cout = 0;
	for(ArgumentBase* a : args){

		if (a->getName().compare("normalized_rt") == 0) {
			ArgumentRT* normalized_rt_arg;
			normalized_rt_arg = (ArgumentRT*)a;
			this->normalized_rt_temp = new DenseDataRegion2D();
			this->normalized_rt_temp->setName(normalized_rt_arg->getName());
			this->normalized_rt_temp->setId(std::to_string(normalized_rt_arg->getId()));
			this->normalized_rt_temp->setVersion(normalized_rt_arg->getId());
			set_cout++;
		}

		if (a->getName().compare("blue") == 0) {
			this->blue = (unsigned char)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("green") == 0) {
			this->green = (unsigned char)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("red") == 0) {
			this->red = (unsigned char)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("T1") == 0) {
			this->T1 = (double)((ArgumentFloat*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("T2") == 0) {
			this->T2 = (double)((ArgumentFloat*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("G1") == 0) {
			this->G1 = (unsigned char)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("minSize") == 0) {
			this->minSize = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("maxSize") == 0) {
			this->maxSize = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("G2") == 0) {
			this->G2 = (unsigned char)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("fillHolesConnectivity") == 0) {
			this->fillHolesConnectivity = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("reconConnectivity") == 0) {
			this->reconConnectivity = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}
	}

	// all arguments except the DataRegions
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;
}

Task1Segmentation::~Task1Segmentation() {
	if(normalized_rt_temp != NULL) delete normalized_rt_temp;
}

bool Task1Segmentation::run(int procType, int tid) {
	
	cv::Mat seg_open_temp;
	cv::Mat normalized_rt = this->normalized_rt_temp->getData();

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "[Task1Segmentation] executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNucleiStg1(normalized_rt, blue, green, 
		red, T1, T2, G1, minSize, maxSize, G2, fillHolesConnectivity, 
		reconConnectivity, &seg_open_temp);
	
	uint64_t t2 = Util::ClockGetTimeProfile();

	seg_open = new cv::Mat(seg_open_temp);

	std::cout << "[Task1Segmentation] time elapsed: "<< t2-t1 << std::endl;
}

void Task1Segmentation::updateDR(RegionTemplate* rt) {
	normalized_rt_temp = dynamic_cast<DenseDataRegion2D*>(rt->getDataRegion(this->normalized_rt_temp->getName(), 
		this->normalized_rt_temp->getId(), 0, stoi(this->normalized_rt_temp->getId())));
}

void Task1Segmentation::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "[Task1Segmentation] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}
}

void Task1Segmentation::resolveDependencies(ReusableTask* t) {}

bool Task1Segmentation::reusable(ReusableTask* rt) {
	Task1Segmentation* t = (Task1Segmentation*)(rt);
	if (this->normalized_rt_temp->getName() == t->normalized_rt_temp->getName() &&
		this->normalized_rt_temp->getId() == t->normalized_rt_temp->getId() &&
		this->normalized_rt_temp->getVersion() == t->normalized_rt_temp->getVersion() &&
		this->blue == t->blue &&
		this->green == t->green &&
		this->red == t->red &&
		this->T1 == t->T1 &&
		this->T2 == t->T2 &&
		this->G1 == t->G1 &&
		this->minSize == t->minSize &&
		this->maxSize == t->maxSize &&
		this->G2 == t->G2 &&
		this->fillHolesConnectivity == t->fillHolesConnectivity &&
		this->reconConnectivity == t->reconConnectivity) {

		return true;
	} else {
		return false;
	}
	return true;
}

int Task1Segmentation::size() {
	return sizeof(unsigned char) + sizeof(unsigned char) + sizeof(unsigned char) + sizeof(double) + 
		sizeof(double) + sizeof(unsigned char) + sizeof(int) + sizeof(int) + sizeof(unsigned char) + 
		sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + 
		sizeof(int) + sizeof(int) + normalized_rt_temp->getName().length()*sizeof(char);
}

int Task1Segmentation::serialize(char *buff) {
	int serialized_bytes = 0;

	// copy id
	int id = this->getId();
	memcpy(buff+serialized_bytes, &id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy parent task id
	int pt = this->parentTask;
	memcpy(buff+serialized_bytes, &pt, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt id
	int normalized_rt_id = stoi(normalized_rt_temp->getId());
	memcpy(buff+serialized_bytes, &normalized_rt_id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt name size
	int normalized_rt_name_size = normalized_rt_temp->getName().length();
	memcpy(buff+serialized_bytes, &normalized_rt_name_size, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt name
	memcpy(buff+serialized_bytes, normalized_rt_temp->getName().c_str(), normalized_rt_name_size*sizeof(char));
	serialized_bytes+=normalized_rt_name_size*sizeof(char);

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
	memcpy(buff+serialized_bytes, &T1, sizeof(double));
	serialized_bytes+=sizeof(double);
		
	// copy field T2
	memcpy(buff+serialized_bytes, &T2, sizeof(double));
	serialized_bytes+=sizeof(double);
		
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

	// copy field fillHolesConnectivity
	memcpy(buff+serialized_bytes, &fillHolesConnectivity, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field reconConnectivity
	memcpy(buff+serialized_bytes, &reconConnectivity, sizeof(int));
	serialized_bytes+=sizeof(int);

	return serialized_bytes;
}

int Task1Segmentation::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// create the normalized_rt
	this->normalized_rt_temp = new DenseDataRegion2D();

	// extract normalized_rt id
	int normalized_rt_id = ((int*)(buff+deserialized_bytes))[0];
	this->normalized_rt_temp->setId(to_string(normalized_rt_id));
	this->normalized_rt_temp->setVersion(normalized_rt_id);
	deserialized_bytes += sizeof(int);

	// extract normalized_rt name size
	int normalized_rt_name_size = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// copy normalized_rt name
	char normalized_rt_name[normalized_rt_name_size+1]; 
	normalized_rt_name[normalized_rt_name_size] = '\0';
	memcpy(normalized_rt_name, buff+deserialized_bytes, sizeof(char)*normalized_rt_name_size);
	deserialized_bytes += sizeof(char)*normalized_rt_name_size;
	this->normalized_rt_temp->setName(normalized_rt_name);

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
	this->T1 = ((double*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(double);

	// extract field T2
	this->T2 = ((double*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(double);

	// extract field G1
	this->G1 = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field minSize
	this->minSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field maxSize
	this->maxSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field G2
	this->G2 = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field fillHolesConnectivity
	this->fillHolesConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field reconConnectivity
	this->reconConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	return deserialized_bytes;
}

ReusableTask* Task1Segmentation::clone() {
	ReusableTask* retValue = new Task1Segmentation();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;

	return retValue;
}

void Task1Segmentation::print() {
	
	cout << "\t\t\tblue: " << int(blue) << endl;
	cout << "\t\t\tgreen: " << int(green) << endl;
	cout << "\t\t\tblue: " << int(red) << endl;
	cout << "\t\t\tT1: " << T1 << endl;
	cout << "\t\t\tT2: " << T2 << endl;
	cout << "\t\t\tG1: " << int(G1) << endl;
	cout << "\t\t\tminSize: " << minSize << endl;
	cout << "\t\t\tmaxSize: " << maxSize << endl;
	cout << "\t\t\tG2: " << int(G2) << endl;
	cout << "\t\t\tfillHolesConnectivity: " << fillHolesConnectivity << endl;
	cout << "\t\t\treconConnectivity: " << reconConnectivity << endl;
	cout << "\t\t\tout-seg_open: " << &seg_open << endl;
}

// Create the task factory
ReusableTask* task1FactorySegmentation1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new Task1Segmentation(args, inputRt);
}

// Create the task factory
ReusableTask* task2FactorySegmentation2() {
	return new Task1Segmentation();
}

// register factory with the runtime system
bool registeredSegmentationTask1 = ReusableTask::ReusableTaskFactory::taskRegister("Task1Segmentation", 
	&task1FactorySegmentation1, &task2FactorySegmentation2);


Task2Segmentation::Task2Segmentation(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	int set_cout = 0;
	for(ArgumentBase* a : args){

		if (a->getName().compare("normalized_rt") == 0) {
			ArgumentRT* normalized_rt_arg;
			normalized_rt_arg = (ArgumentRT*)a;
			this->normalized_rt_temp = new DenseDataRegion2D();
			this->normalized_rt_temp->setName(normalized_rt_arg->getName());
			this->normalized_rt_temp->setId(std::to_string(normalized_rt_arg->getId()));
			this->normalized_rt_temp->setVersion(normalized_rt_arg->getId());
			set_cout++;
		}

		if (a->getName().compare("minSizePl") == 0) {
			this->minSizePl = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("watershedConnectivity") == 0) {
			this->watershedConnectivity = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}
	}

	// all arguments except the DataRegions
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;
}

Task2Segmentation::~Task2Segmentation() {
	if(normalized_rt_temp != NULL) delete normalized_rt_temp;
}

bool Task2Segmentation::run(int procType, int tid) {
	
	cv::Mat seg_nonoverlap_temp;
	cv::Mat normalized_rt = this->normalized_rt_temp->getData();

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "[Task2Segmentation] executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNucleiStg2(normalized_rt, minSizePl, watershedConnectivity, *seg_open, &seg_nonoverlap_temp);

	uint64_t t2 = Util::ClockGetTimeProfile();
	seg_nonoverlap = new cv::Mat(seg_nonoverlap_temp);

	std::cout << "[Task2Segmentation] time elapsed: "<< t2-t1 << std::endl;
}

void Task2Segmentation::updateDR(RegionTemplate* rt) {
	normalized_rt_temp = dynamic_cast<DenseDataRegion2D*>(rt->getDataRegion(this->normalized_rt_temp->getName(), 
		this->normalized_rt_temp->getId(), 0, stoi(this->normalized_rt_temp->getId())));
}

void Task2Segmentation::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "[Task2Segmentation] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}

	this->seg_open = ((Task2Segmentation*)t)->seg_open;
}

void Task2Segmentation::resolveDependencies(ReusableTask* t) {
	// verify if the task type is compatible
	if (typeid(t) != typeid(Task1Segmentation)) {
		std::cout << "[Task2Segmentation] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
	}
	this->seg_open = &((Task1Segmentation*)t)->seg_open;
}

bool Task2Segmentation::reusable(ReusableTask* rt) {
	Task2Segmentation* t = (Task2Segmentation*)(rt);
	if (this->normalized_rt_temp->getName() == t->normalized_rt_temp->getName() &&
		this->normalized_rt_temp->getId() == t->normalized_rt_temp->getId() &&
		this->normalized_rt_temp->getVersion() == t->normalized_rt_temp->getVersion() &&
		this->minSizePl == t->minSizePl &&
		this->watershedConnectivity == t->watershedConnectivity) {

		return true;
	} else {
		return false;
	}
	return true;
}

int Task2Segmentation::size() {
	return sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) +
		sizeof(int) + sizeof(int) + normalized_rt_temp->getName().length()*sizeof(char);
}

int Task2Segmentation::serialize(char *buff) {
	int serialized_bytes = 0;

	// copy id
	int id = this->getId();
	memcpy(buff+serialized_bytes, &id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy parent task id
	int pt = this->parentTask;
	memcpy(buff+serialized_bytes, &pt, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt id
	int normalized_rt_id = stoi(normalized_rt_temp->getId());
	memcpy(buff+serialized_bytes, &normalized_rt_id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt name size
	int normalized_rt_name_size = normalized_rt_temp->getName().length();
	memcpy(buff+serialized_bytes, &normalized_rt_name_size, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt name
	memcpy(buff+serialized_bytes, normalized_rt_temp->getName().c_str(), normalized_rt_name_size*sizeof(char));
	serialized_bytes+=normalized_rt_name_size*sizeof(char);
		
	// copy field minSizePl
	memcpy(buff+serialized_bytes, &minSizePl, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field watershedConnectivity
	memcpy(buff+serialized_bytes, &watershedConnectivity, sizeof(int));
	serialized_bytes+=sizeof(int);

	return serialized_bytes;
}

int Task2Segmentation::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// create the normalized_rt
	this->normalized_rt_temp = new DenseDataRegion2D();

	// extract normalized_rt id
	int normalized_rt_id = ((int*)(buff+deserialized_bytes))[0];
	this->normalized_rt_temp->setId(to_string(normalized_rt_id));
	this->normalized_rt_temp->setVersion(normalized_rt_id);
	deserialized_bytes += sizeof(int);

	// extract normalized_rt name size
	int normalized_rt_name_size = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// copy normalized_rt name
	char normalized_rt_name[normalized_rt_name_size+1]; 
	normalized_rt_name[normalized_rt_name_size] = '\0';
	memcpy(normalized_rt_name, buff+deserialized_bytes, sizeof(char)*normalized_rt_name_size);
	deserialized_bytes += sizeof(char)*normalized_rt_name_size;
	this->normalized_rt_temp->setName(normalized_rt_name);

	// extract field minSizePl
	this->minSizePl = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field watershedConnectivity
	this->watershedConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	return deserialized_bytes;
}

ReusableTask* Task2Segmentation::clone() {
	ReusableTask* retValue = new Task2Segmentation();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;

	return retValue;
}

void Task2Segmentation::print() {
	
	cout << "\t\t\tminSizePl: " << minSizePl << endl;
	cout << "\t\t\twatershedConnectivity: " << watershedConnectivity << endl;
	cout << "\t\t\tin-seg_open: " << seg_open << endl;
	cout << "\t\t\tout-seg_nonoverlap: " << &seg_nonoverlap << endl;
}

// Create the task factory
ReusableTask* task2FactorySegmentation1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new Task2Segmentation(args, inputRt);
}

// Create the task factory
ReusableTask* taskFactorySegmentation2() {
	return new Task2Segmentation();
}

// register factory with the runtime system
bool registeredSegmentationTask2 = ReusableTask::ReusableTaskFactory::taskRegister("Task2Segmentation", 
	&task2FactorySegmentation1, &taskFactorySegmentation2);

Task3Segmentation::Task3Segmentation(list<ArgumentBase*> args, RegionTemplate* inputRt) {

	int set_cout = 0;
	for(ArgumentBase* a : args){

		if (a->getName().compare("segmented_rt") == 0) {
			ArgumentRT* segmented_rt_arg;
			segmented_rt_arg = (ArgumentRT*)a;
			this->segmented_rt_temp = new DenseDataRegion2D();
			this->segmented_rt_temp->setName(segmented_rt_arg->getName());
			this->segmented_rt_temp->setId(std::to_string(segmented_rt_arg->getId()));
			this->segmented_rt_temp->setVersion(segmented_rt_arg->getId());
			set_cout++;
		}

		if (a->getName().compare("minSizeSeg") == 0) {
			this->minSizeSeg = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("maxSizeSeg") == 0) {
			this->maxSizeSeg = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("fillHolesConnectivity") == 0) {
			this->fillHolesConnectivity = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

	}

	// all arguments except the DataRegions
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;
}

Task3Segmentation::~Task3Segmentation() {
	// if(segmented_rt_temp != NULL) delete segmented_rt_temp;
}

bool Task3Segmentation::run(int procType, int tid) {
	
	cv::Mat segmented_rt;

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "[Task3Segmentation] executing." << std::endl;

	::nscale::HistologicalEntities::segmentNucleiStg3(minSizeSeg, maxSizeSeg, fillHolesConnectivity, *seg_nonoverlap, &segmented_rt);
	
	this->segmented_rt_temp->setData(segmented_rt);

	uint64_t t2 = Util::ClockGetTimeProfile();

	std::cout << "[Task3Segmentation] time elapsed: "<< t2-t1 << std::endl;
}

void Task3Segmentation::updateDR(RegionTemplate* rt) {
	rt->insertDataRegion(this->segmented_rt_temp);
}

void Task3Segmentation::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "[Task3Segmentation] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}

	this->seg_nonoverlap = ((Task3Segmentation*)t)->seg_nonoverlap;
}

void Task3Segmentation::resolveDependencies(ReusableTask* t) {
	// verify if the task type is compatible
	if (typeid(t) != typeid(Task2Segmentation)) {
		std::cout << "[Task3Segmentation] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
	}
	this->seg_nonoverlap = &((Task2Segmentation*)t)->seg_nonoverlap;
}

bool Task3Segmentation::reusable(ReusableTask* rt) {
	Task3Segmentation* t = (Task3Segmentation*)(rt);
	if (this->segmented_rt_temp->getName() == t->segmented_rt_temp->getName() &&
		this->segmented_rt_temp->getId() == t->segmented_rt_temp->getId() &&
		this->segmented_rt_temp->getVersion() == t->segmented_rt_temp->getVersion() &&
		this->minSizeSeg == t->minSizeSeg &&
		this->maxSizeSeg == t->maxSizeSeg &&
		this->fillHolesConnectivity == t->fillHolesConnectivity) {

		return true;
	} else {
		return false;
	}
	return true;
}

int Task3Segmentation::size() {
	return sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + 
		sizeof(int) + sizeof(int) + segmented_rt_temp->getName().length()*sizeof(char);
}

int Task3Segmentation::serialize(char *buff) {
	int serialized_bytes = 0;

	// copy id
	int id = this->getId();
	memcpy(buff+serialized_bytes, &id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy parent task id
	int pt = this->parentTask;
	memcpy(buff+serialized_bytes, &pt, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy segmented_rt id
	int segmented_rt_id = stoi(segmented_rt_temp->getId());
	memcpy(buff+serialized_bytes, &segmented_rt_id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy segmented_rt name size
	int segmented_rt_name_size = segmented_rt_temp->getName().length();
	memcpy(buff+serialized_bytes, &segmented_rt_name_size, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy segmented_rt name
	memcpy(buff+serialized_bytes, segmented_rt_temp->getName().c_str(), segmented_rt_name_size*sizeof(char));
	serialized_bytes+=segmented_rt_name_size*sizeof(char);

	// copy field minSizeSeg
	memcpy(buff+serialized_bytes, &minSizeSeg, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field maxSizeSeg
	memcpy(buff+serialized_bytes, &maxSizeSeg, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field fillHolesConnectivity
	memcpy(buff+serialized_bytes, &fillHolesConnectivity, sizeof(int));
	serialized_bytes+=sizeof(int);

	return serialized_bytes;
}

int Task3Segmentation::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// create the segmented_rt
	this->segmented_rt_temp = new DenseDataRegion2D();

	// extract segmented_rt id
	int segmented_rt_id = ((int*)(buff+deserialized_bytes))[0];
	this->segmented_rt_temp->setId(to_string(segmented_rt_id));
	this->segmented_rt_temp->setVersion(segmented_rt_id);
	deserialized_bytes += sizeof(int);

	// extract segmented_rt name size
	int segmented_rt_name_size = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// copy segmented_rt name
	char segmented_rt_name[segmented_rt_name_size+1]; 
	segmented_rt_name[segmented_rt_name_size] = '\0';
	memcpy(segmented_rt_name, buff+deserialized_bytes, sizeof(char)*segmented_rt_name_size);
	deserialized_bytes += sizeof(char)*segmented_rt_name_size;
	this->segmented_rt_temp->setName(segmented_rt_name);

	// extract field minSizeSeg
	this->minSizeSeg = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field maxSizeSeg
	this->maxSizeSeg = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field fillHolesConnectivity
	this->fillHolesConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	return deserialized_bytes;
}

ReusableTask* Task3Segmentation::clone() {
	ReusableTask* retValue = new Task3Segmentation();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;

	return retValue;
}

void Task3Segmentation::print() {
	
	cout << "\t\t\tminSizeSeg: " << minSizeSeg << endl;
	cout << "\t\t\tmaxSizeSeg: " << maxSizeSeg << endl;
	cout << "\t\t\tfillHolesConnectivity: " << fillHolesConnectivity << endl;
	cout << "\t\t\tin-seg_nonoverlap: " << seg_nonoverlap << endl;
}

// Create the task factory
ReusableTask* task3FactorySegmentation1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new Task3Segmentation(args, inputRt);
}

// Create the task factory
ReusableTask* task3FactorySegmentation2() {
	return new Task3Segmentation();
}

// register factory with the runtime system
bool registeredSegmentationTask3 = ReusableTask::ReusableTaskFactory::taskRegister("Task3Segmentation", 
	&task3FactorySegmentation1, &task3FactorySegmentation2);
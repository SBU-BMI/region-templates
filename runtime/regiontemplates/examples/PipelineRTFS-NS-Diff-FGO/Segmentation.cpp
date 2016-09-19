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
	list<ArgumentBase*> task_Segmentation0_args;
	list<ArgumentBase*> task_Segmentation1_args;
	list<ArgumentBase*> task_Segmentation2_args;
	list<ArgumentBase*> task_Segmentation3_args;
	list<ArgumentBase*> task_Segmentation4_args;
	list<ArgumentBase*> task_Segmentation5_args;
	list<ArgumentBase*> task_Segmentation6_args;

	ArgumentRT* normalized_rt0 = new ArgumentRT();
	normalized_rt0->setName("normalized_rt");
	task_Segmentation0_args.emplace_back(normalized_rt0);
	ArgumentInt* blue0 = new ArgumentInt();
	blue0->setName("blue");
	task_Segmentation0_args.emplace_back(blue0);
	ArgumentInt* green0 = new ArgumentInt();
	green0->setName("green");
	task_Segmentation0_args.emplace_back(green0);
	ArgumentInt* red0 = new ArgumentInt();
	red0->setName("red");
	task_Segmentation0_args.emplace_back(red0);
	ArgumentFloat* T10 = new ArgumentFloat();
	T10->setName("T1");
	task_Segmentation0_args.emplace_back(T10);
	ArgumentFloat* T20 = new ArgumentFloat();
	T20->setName("T2");
	task_Segmentation0_args.emplace_back(T20);
	this->tasksDesc["TaskSegmentation0"] = task_Segmentation0_args;

	ArgumentInt* reconConnectivity1 = new ArgumentInt();
	reconConnectivity1->setName("reconConnectivity");
	task_Segmentation1_args.emplace_back(reconConnectivity1);
	this->tasksDesc["TaskSegmentation1"] = task_Segmentation1_args;

	ArgumentInt* fillHolesConnectivity2 = new ArgumentInt();
	fillHolesConnectivity2->setName("fillHolesConnectivity");
	task_Segmentation2_args.emplace_back(fillHolesConnectivity2);
	ArgumentInt* G12 = new ArgumentInt();
	G12->setName("G1");
	task_Segmentation2_args.emplace_back(G12);
	this->tasksDesc["TaskSegmentation2"] = task_Segmentation2_args;

	ArgumentInt* minSize3 = new ArgumentInt();
	minSize3->setName("minSize");
	task_Segmentation3_args.emplace_back(minSize3);
	ArgumentInt* maxSize3 = new ArgumentInt();
	maxSize3->setName("maxSize");
	task_Segmentation3_args.emplace_back(maxSize3);
	this->tasksDesc["TaskSegmentation3"] = task_Segmentation3_args;

	ArgumentInt* G24 = new ArgumentInt();
	G24->setName("G2");
	task_Segmentation4_args.emplace_back(G24);
	this->tasksDesc["TaskSegmentation4"] = task_Segmentation4_args;

	ArgumentRT* normalized_rt5 = new ArgumentRT();
	normalized_rt5->setName("normalized_rt");
	task_Segmentation5_args.emplace_back(normalized_rt5);
	ArgumentInt* minSizePl5 = new ArgumentInt();
	minSizePl5->setName("minSizePl");
	task_Segmentation5_args.emplace_back(minSizePl5);
	ArgumentInt* watershedConnectivity5 = new ArgumentInt();
	watershedConnectivity5->setName("watershedConnectivity");
	task_Segmentation5_args.emplace_back(watershedConnectivity5);
	this->tasksDesc["TaskSegmentation5"] = task_Segmentation5_args;

	ArgumentRT* segmented_rt6 = new ArgumentRT();
	segmented_rt6->setName("segmented_rt");
	task_Segmentation6_args.emplace_back(segmented_rt6);
	ArgumentInt* minSizeSeg6 = new ArgumentInt();
	minSizeSeg6->setName("minSizeSeg");
	task_Segmentation6_args.emplace_back(minSizeSeg6);
	ArgumentInt* maxSizeSeg6 = new ArgumentInt();
	maxSizeSeg6->setName("maxSizeSeg");
	task_Segmentation6_args.emplace_back(maxSizeSeg6);
	ArgumentInt* fillHolesConnectivity6 = new ArgumentInt();
	fillHolesConnectivity6->setName("fillHolesConnectivity");
	task_Segmentation6_args.emplace_back(fillHolesConnectivity6);
	this->tasksDesc["TaskSegmentation6"] = task_Segmentation6_args;


}

Segmentation::~Segmentation() {}

int Segmentation::run() {

	// Print name and id of the component instance
	std::cout << "\t\t\tExecuting component: " << this->getComponentName() << " instance id: " << this->getId() <<std::endl;
	RegionTemplate * inputRt = this->getRegionTemplateInstance("tile");

	this->addInputOutputDataRegion("tile", "normalized_rt", RTPipelineComponentBase::INPUT);

	this->addInputOutputDataRegion("tile", "segmented_rt", RTPipelineComponentBase::OUTPUT);


	map<int, ReusableTask*> prev_map;
	for (list<ReusableTask*>::reverse_iterator task=tasks.rbegin(); task!=tasks.rend(); task++) {
		cout << "\t\t\t[Segmentation] sending task " << (*task)->getId() << endl;
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

TaskSegmentation0::TaskSegmentation0() {
	bgr = std::shared_ptr<std::vector<cv::Mat>>(new std::vector<cv::Mat>());
	rbc = std::shared_ptr<cv::Mat>(new cv::Mat());

}

TaskSegmentation0::TaskSegmentation0(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	
	int set_cout = 0;
	for(ArgumentBase* a : args){
		if (a->getName().compare("normalized_rt") == 0) {
			ArgumentRT* normalized_rt_arg;
			normalized_rt_arg = (ArgumentRT*)a;
			this->normalized_rt_temp = std::make_shared<DenseDataRegion2D*>(new DenseDataRegion2D());
			(*this->normalized_rt_temp)->setName(normalized_rt_arg->getName());
			(*this->normalized_rt_temp)->setId(std::to_string(normalized_rt_arg->getId()));
			(*this->normalized_rt_temp)->setVersion(normalized_rt_arg->getId());
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


	}
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;

	bgr = std::shared_ptr<std::vector<cv::Mat>>(new std::vector<cv::Mat>());
	rbc = std::shared_ptr<cv::Mat>(new cv::Mat());
	
}

TaskSegmentation0::~TaskSegmentation0() {
}

bool TaskSegmentation0::run(int procType, int tid) {
	cv::Mat normalized_rt = (*this->normalized_rt_temp)->getData();

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "\t\t\tTaskSegmentation0 executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNucleiStg1(normalized_rt, blue, green, red, T1, T2, &(*bgr), &(*rbc));
	
	uint64_t t2 = Util::ClockGetTimeProfile();


	std::cout << "\t\t\tTask Segmentation0 time elapsed: "<< t2-t1 << std::endl;
}

void TaskSegmentation0::updateDR(RegionTemplate* rt) {
	normalized_rt_temp = std::make_shared<DenseDataRegion2D*>(dynamic_cast<DenseDataRegion2D*>(
		rt->getDataRegion((*this->normalized_rt_temp)->getName(),
		(*this->normalized_rt_temp)->getId(), 0, stoi((*this->normalized_rt_temp)->getId()))));

}

void TaskSegmentation0::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "\t\t\t[TaskSegmentation0] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}


}

void TaskSegmentation0::resolveDependencies(ReusableTask* t) {
	// verify if the task type is compatible
	if (typeid(t) != typeid(TaskSegmentation1)) {
		std::cout << "\t\t\t[TaskSegmentation1] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
	}

	

}

bool TaskSegmentation0::reusable(ReusableTask* rt) {
	TaskSegmentation0* t = (TaskSegmentation0*)(rt);
	if (
		(*this->normalized_rt_temp)->getName() == (*t->normalized_rt_temp)->getName() &&
		this->blue == t->blue &&
		this->green == t->green &&
		this->red == t->red &&
		this->T1 == t->T1 &&
		this->T2 == t->T2 &&

		true) {

		return true;
	} else {
		return false;
	}
	return true;
}

int TaskSegmentation0::size() {
	return 
		sizeof(int) + sizeof(int) +
		sizeof(int) + (*normalized_rt_temp)->getName().length()*sizeof(char) + sizeof(int) +
		sizeof(unsigned char) +
		sizeof(unsigned char) +
		sizeof(unsigned char) +
		sizeof(double) +
		sizeof(double) +

		0;
}

int TaskSegmentation0::serialize(char *buff) {
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
	int normalized_rt_id = stoi((*normalized_rt_temp)->getId());
	memcpy(buff+serialized_bytes, &normalized_rt_id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt name size
	int normalized_rt_name_size = (*normalized_rt_temp)->getName().length();
	memcpy(buff+serialized_bytes, &normalized_rt_name_size, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt name
	memcpy(buff+serialized_bytes, (*normalized_rt_temp)->getName().c_str(), normalized_rt_name_size*sizeof(char));
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


	return serialized_bytes;
}

int TaskSegmentation0::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// create the normalized_rt
	this->normalized_rt_temp = std::make_shared<DenseDataRegion2D*>(new DenseDataRegion2D());

	// extract normalized_rt id
	int normalized_rt_id = ((int*)(buff+deserialized_bytes))[0];
	(*this->normalized_rt_temp)->setId(to_string(normalized_rt_id));
	(*this->normalized_rt_temp)->setVersion(normalized_rt_id);
	deserialized_bytes += sizeof(int);

	// extract normalized_rt name size
	int normalized_rt_name_size = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// copy normalized_rt name
	char normalized_rt_name[normalized_rt_name_size+1];
	normalized_rt_name[normalized_rt_name_size] = '\0';
	memcpy(normalized_rt_name, buff+deserialized_bytes, sizeof(char)*normalized_rt_name_size);
	deserialized_bytes += sizeof(char)*normalized_rt_name_size;
	(*this->normalized_rt_temp)->setName(normalized_rt_name);

	// extract field blue
	this->blue = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field green
	this->green = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field red
	this->red = ((unsigned char*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(unsigned char);

	// extract field T1
	this->T1 = ((double*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(double);

	// extract field T2
	this->T2 = ((double*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(double);


	return deserialized_bytes;
}

ReusableTask* TaskSegmentation0::clone() {
	ReusableTask* retValue = new TaskSegmentation0();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete[] buff;

	return retValue;
}

void TaskSegmentation0::print() {
	cout << "\t\t\tblue: " << blue << endl;
	cout << "\t\t\tgreen: " << green << endl;
	cout << "\t\t\tred: " << red << endl;
	cout << "\t\t\tT1: " << T1 << endl;
	cout << "\t\t\tT2: " << T2 << endl;

}

// Create the task factory
ReusableTask* Segmentation0Factory1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new TaskSegmentation0(args, inputRt);
}

// Create the task factory
ReusableTask* Segmentation0Factory2() {
	return new TaskSegmentation0();
}

// register factory with the runtime system
bool registeredTaskSegmentation02 = ReusableTask::ReusableTaskFactory::taskRegister("TaskSegmentation0", 
	&Segmentation0Factory1, &Segmentation0Factory2);

TaskSegmentation1::TaskSegmentation1() {
	rc = std::shared_ptr<cv::Mat>(new cv::Mat());
	rc_recon = std::shared_ptr<cv::Mat>(new cv::Mat());
	rc_open = std::shared_ptr<cv::Mat>(new cv::Mat());

}

TaskSegmentation1::TaskSegmentation1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	
	int set_cout = 0;
	for(ArgumentBase* a : args){
		if (a->getName().compare("reconConnectivity") == 0) {
			this->reconConnectivity = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}


	}
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;

	rc = std::shared_ptr<cv::Mat>(new cv::Mat());
	rc_recon = std::shared_ptr<cv::Mat>(new cv::Mat());
	rc_open = std::shared_ptr<cv::Mat>(new cv::Mat());
	
}

TaskSegmentation1::~TaskSegmentation1() {
}

bool TaskSegmentation1::run(int procType, int tid) {

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "\t\t\tTaskSegmentation1 executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNucleiStg2(reconConnectivity, &*bgr, &*rc, &*rc_recon, &*rc_open);
	
	uint64_t t2 = Util::ClockGetTimeProfile();


	std::cout << "\t\t\tTask Segmentation1 time elapsed: "<< t2-t1 << std::endl;
}

void TaskSegmentation1::updateDR(RegionTemplate* rt) {

}

void TaskSegmentation1::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "\t\t\t[TaskSegmentation1] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}

	this->bgr = ((TaskSegmentation1*)t)->bgr;
	this->rbc_fw = ((TaskSegmentation1*)t)->rbc_fw;

}

void TaskSegmentation1::resolveDependencies(ReusableTask* t) {
	// verify if the task type is compatible
	if (typeid(t) != typeid(TaskSegmentation2)) {
		std::cout << "\t\t\t[TaskSegmentation2] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
	}

	
	this->bgr = ((TaskSegmentation0*)t)->bgr;
	this->rbc_fw = ((TaskSegmentation0*)t)->rbc;

}

bool TaskSegmentation1::reusable(ReusableTask* rt) {
	TaskSegmentation1* t = (TaskSegmentation1*)(rt);
	if (
		this->reconConnectivity == t->reconConnectivity &&

		true) {

		return true;
	} else {
		return false;
	}
	return true;
}

int TaskSegmentation1::size() {
	return 
		sizeof(int) + sizeof(int) +
		sizeof(int) +

		0;
}

int TaskSegmentation1::serialize(char *buff) {
	int serialized_bytes = 0;
	// copy id
	int id = this->getId();
	memcpy(buff+serialized_bytes, &id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy parent task id
	int pt = this->parentTask;
	memcpy(buff+serialized_bytes, &pt, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field reconConnectivity
	memcpy(buff+serialized_bytes, &reconConnectivity, sizeof(int));
	serialized_bytes+=sizeof(int);


	return serialized_bytes;
}

int TaskSegmentation1::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field reconConnectivity
	this->reconConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);


	return deserialized_bytes;
}

ReusableTask* TaskSegmentation1::clone() {
	ReusableTask* retValue = new TaskSegmentation1();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete[] buff;

	return retValue;
}

void TaskSegmentation1::print() {
	cout << "\t\t\treconConnectivity: " << reconConnectivity << endl;

}

// Create the task factory
ReusableTask* Segmentation1Factory1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new TaskSegmentation1(args, inputRt);
}

// Create the task factory
ReusableTask* Segmentation1Factory2() {
	return new TaskSegmentation1();
}

// register factory with the runtime system
bool registeredTaskSegmentation12 = ReusableTask::ReusableTaskFactory::taskRegister("TaskSegmentation1", 
	&Segmentation1Factory1, &Segmentation1Factory2);

TaskSegmentation2::TaskSegmentation2() {
	bw1 = std::shared_ptr<cv::Mat>(new cv::Mat());
	diffIm = std::shared_ptr<cv::Mat>(new cv::Mat());

}

TaskSegmentation2::TaskSegmentation2(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	
	int set_cout = 0;
	for(ArgumentBase* a : args){
		if (a->getName().compare("fillHolesConnectivity") == 0) {
			this->fillHolesConnectivity = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("G1") == 0) {
			this->G1 = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}


	}
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;

	bw1 = std::shared_ptr<cv::Mat>(new cv::Mat());
	diffIm = std::shared_ptr<cv::Mat>(new cv::Mat());
	
}

TaskSegmentation2::~TaskSegmentation2() {
}

bool TaskSegmentation2::run(int procType, int tid) {

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "\t\t\tTaskSegmentation2 executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNucleiStg3(fillHolesConnectivity, G1, &*rc, &*rc_recon, &*rc_open, &*bw1, &*diffIm);
	
	uint64_t t2 = Util::ClockGetTimeProfile();


	std::cout << "\t\t\tTask Segmentation2 time elapsed: "<< t2-t1 << std::endl;
}

void TaskSegmentation2::updateDR(RegionTemplate* rt) {

}

void TaskSegmentation2::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "\t\t\t[TaskSegmentation2] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}

	this->rc = ((TaskSegmentation2*)t)->rc;
	this->rc_recon = ((TaskSegmentation2*)t)->rc_recon;
	this->rc_open = ((TaskSegmentation2*)t)->rc_open;
	this->rbc_fw = ((TaskSegmentation2*)t)->rbc_fw;

}

void TaskSegmentation2::resolveDependencies(ReusableTask* t) {
	// verify if the task type is compatible
	if (typeid(t) != typeid(TaskSegmentation3)) {
		std::cout << "\t\t\t[TaskSegmentation3] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
	}

	
	this->rc = ((TaskSegmentation1*)t)->rc;
	this->rc_recon = ((TaskSegmentation1*)t)->rc_recon;
	this->rc_open = ((TaskSegmentation1*)t)->rc_open;
	this->rbc_fw = ((TaskSegmentation1*)t)->rbc_fw;

}

bool TaskSegmentation2::reusable(ReusableTask* rt) {
	TaskSegmentation2* t = (TaskSegmentation2*)(rt);
	if (
		this->fillHolesConnectivity == t->fillHolesConnectivity &&
		this->G1 == t->G1 &&

		true) {

		return true;
	} else {
		return false;
	}
	return true;
}

int TaskSegmentation2::size() {
	return 
		sizeof(int) + sizeof(int) +
		sizeof(int) +
		sizeof(int) +

		0;
}

int TaskSegmentation2::serialize(char *buff) {
	int serialized_bytes = 0;
	// copy id
	int id = this->getId();
	memcpy(buff+serialized_bytes, &id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy parent task id
	int pt = this->parentTask;
	memcpy(buff+serialized_bytes, &pt, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field fillHolesConnectivity
	memcpy(buff+serialized_bytes, &fillHolesConnectivity, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field G1
	memcpy(buff+serialized_bytes, &G1, sizeof(int));
	serialized_bytes+=sizeof(int);


	return serialized_bytes;
}

int TaskSegmentation2::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field fillHolesConnectivity
	this->fillHolesConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field G1
	this->G1 = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);


	return deserialized_bytes;
}

ReusableTask* TaskSegmentation2::clone() {
	ReusableTask* retValue = new TaskSegmentation2();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete[] buff;

	return retValue;
}

void TaskSegmentation2::print() {
	cout << "\t\t\tfillHolesConnectivity: " << fillHolesConnectivity << endl;
	cout << "\t\t\tG1: " << G1 << endl;

}

// Create the task factory
ReusableTask* Segmentation2Factory1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new TaskSegmentation2(args, inputRt);
}

// Create the task factory
ReusableTask* Segmentation2Factory2() {
	return new TaskSegmentation2();
}

// register factory with the runtime system
bool registeredTaskSegmentation22 = ReusableTask::ReusableTaskFactory::taskRegister("TaskSegmentation2", 
	&Segmentation2Factory1, &Segmentation2Factory2);

TaskSegmentation3::TaskSegmentation3() {
	bw1_t = std::shared_ptr<cv::Mat>(new cv::Mat());

}

TaskSegmentation3::TaskSegmentation3(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	
	int set_cout = 0;
	for(ArgumentBase* a : args){
		if (a->getName().compare("minSize") == 0) {
			this->minSize = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}

		if (a->getName().compare("maxSize") == 0) {
			this->maxSize = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}


	}
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;

	bw1_t = std::shared_ptr<cv::Mat>(new cv::Mat());
	
}

TaskSegmentation3::~TaskSegmentation3() {
}

bool TaskSegmentation3::run(int procType, int tid) {


	cv::Mat bw1_t_temp;

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "\t\t\tTaskSegmentation3 executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNucleiStg4(minSize, maxSize, &*bw1, &*bw1_t);
	
	uint64_t t2 = Util::ClockGetTimeProfile();


	std::cout << "\t\t\tTask Segmentation3 time elapsed: "<< t2-t1 << std::endl;
}

void TaskSegmentation3::updateDR(RegionTemplate* rt) {

}

void TaskSegmentation3::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "\t\t\t[TaskSegmentation3] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}

	this->bw1 = ((TaskSegmentation3*)t)->bw1;
	this->rbc_fw = ((TaskSegmentation3*)t)->rbc_fw;
	this->diffIm_fw = ((TaskSegmentation3*)t)->diffIm_fw;

}

void TaskSegmentation3::resolveDependencies(ReusableTask* t) {
	// verify if the task type is compatible
	if (typeid(t) != typeid(TaskSegmentation4)) {
		std::cout << "\t\t\t[TaskSegmentation4] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
	}

	
	this->bw1 = ((TaskSegmentation2*)t)->bw1;
	this->rbc_fw = ((TaskSegmentation2*)t)->rbc_fw;
	this->diffIm_fw = ((TaskSegmentation2*)t)->diffIm;

}

bool TaskSegmentation3::reusable(ReusableTask* rt) {
	TaskSegmentation3* t = (TaskSegmentation3*)(rt);
	if (
		this->minSize == t->minSize &&
		this->maxSize == t->maxSize &&

		true) {

		return true;
	} else {
		return false;
	}
	return true;
}

int TaskSegmentation3::size() {
	return 
		sizeof(int) + sizeof(int) +
		sizeof(int) +
		sizeof(int) +

		0;
}

int TaskSegmentation3::serialize(char *buff) {
	int serialized_bytes = 0;
	// copy id
	int id = this->getId();
	memcpy(buff+serialized_bytes, &id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy parent task id
	int pt = this->parentTask;
	memcpy(buff+serialized_bytes, &pt, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field minSize
	memcpy(buff+serialized_bytes, &minSize, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field maxSize
	memcpy(buff+serialized_bytes, &maxSize, sizeof(int));
	serialized_bytes+=sizeof(int);


	return serialized_bytes;
}

int TaskSegmentation3::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field minSize
	this->minSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field maxSize
	this->maxSize = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);


	return deserialized_bytes;
}

ReusableTask* TaskSegmentation3::clone() {
	ReusableTask* retValue = new TaskSegmentation3();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete[] buff;

	return retValue;
}

void TaskSegmentation3::print() {
	cout << "\t\t\tminSize: " << minSize << endl;
	cout << "\t\t\tmaxSize: " << maxSize << endl;

}

// Create the task factory
ReusableTask* Segmentation3Factory1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new TaskSegmentation3(args, inputRt);
}

// Create the task factory
ReusableTask* Segmentation3Factory2() {
	return new TaskSegmentation3();
}

// register factory with the runtime system
bool registeredTaskSegmentation32 = ReusableTask::ReusableTaskFactory::taskRegister("TaskSegmentation3", 
	&Segmentation3Factory1, &Segmentation3Factory2);

TaskSegmentation4::TaskSegmentation4() {
	seg_open = std::shared_ptr<cv::Mat>(new cv::Mat());

}

TaskSegmentation4::TaskSegmentation4(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	
	int set_cout = 0;
	for(ArgumentBase* a : args){
		if (a->getName().compare("G2") == 0) {
			this->G2 = (int)((ArgumentInt*)a)->getArgValue();
			set_cout++;
		}


	}
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;

	seg_open = std::shared_ptr<cv::Mat>(new cv::Mat());
	
}

TaskSegmentation4::~TaskSegmentation4() {
}

bool TaskSegmentation4::run(int procType, int tid) {


	cv::Mat seg_open_temp;

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "\t\t\tTaskSegmentation4 executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNucleiStg5(G2, &*diffIm, &*bw1_t, &*rbc, &*seg_open);
	
	uint64_t t2 = Util::ClockGetTimeProfile();


	std::cout << "\t\t\tTask Segmentation4 time elapsed: "<< t2-t1 << std::endl;
}

void TaskSegmentation4::updateDR(RegionTemplate* rt) {

}

void TaskSegmentation4::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "\t\t\t[TaskSegmentation4] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}

	this->diffIm = ((TaskSegmentation4*)t)->diffIm;
	this->bw1_t = ((TaskSegmentation4*)t)->bw1_t;
	this->rbc = ((TaskSegmentation4*)t)->rbc;

}

void TaskSegmentation4::resolveDependencies(ReusableTask* t) {
	// verify if the task type is compatible
	if (typeid(t) != typeid(TaskSegmentation5)) {
		std::cout << "\t\t\t[TaskSegmentation5] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
	}

	
	this->diffIm = ((TaskSegmentation3*)t)->diffIm_fw;
	this->bw1_t = ((TaskSegmentation3*)t)->bw1_t;
	this->rbc = ((TaskSegmentation3*)t)->rbc_fw;

}

bool TaskSegmentation4::reusable(ReusableTask* rt) {
	TaskSegmentation4* t = (TaskSegmentation4*)(rt);
	if (
		this->G2 == t->G2 &&

		true) {

		return true;
	} else {
		return false;
	}
	return true;
}

int TaskSegmentation4::size() {
	return 
		sizeof(int) + sizeof(int) +
		sizeof(int) +

		0;
}

int TaskSegmentation4::serialize(char *buff) {
	int serialized_bytes = 0;
	// copy id
	int id = this->getId();
	memcpy(buff+serialized_bytes, &id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy parent task id
	int pt = this->parentTask;
	memcpy(buff+serialized_bytes, &pt, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field G2
	memcpy(buff+serialized_bytes, &G2, sizeof(int));
	serialized_bytes+=sizeof(int);


	return serialized_bytes;
}

int TaskSegmentation4::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field G2
	this->G2 = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);


	return deserialized_bytes;
}

ReusableTask* TaskSegmentation4::clone() {
	ReusableTask* retValue = new TaskSegmentation4();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete[] buff;

	return retValue;
}

void TaskSegmentation4::print() {
	cout << "\t\t\tG2: " << G2 << endl;

}

// Create the task factory
ReusableTask* Segmentation4Factory1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new TaskSegmentation4(args, inputRt);
}

// Create the task factory
ReusableTask* Segmentation4Factory2() {
	return new TaskSegmentation4();
}

// register factory with the runtime system
bool registeredTaskSegmentation42 = ReusableTask::ReusableTaskFactory::taskRegister("TaskSegmentation4", 
	&Segmentation4Factory1, &Segmentation4Factory2);

TaskSegmentation5::TaskSegmentation5() {
	seg_nonoverlap = std::shared_ptr<cv::Mat>(new cv::Mat());

}

TaskSegmentation5::TaskSegmentation5(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	
	int set_cout = 0;
	for(ArgumentBase* a : args){
		if (a->getName().compare("normalized_rt") == 0) {
			ArgumentRT* normalized_rt_arg;
			normalized_rt_arg = (ArgumentRT*)a;
			this->normalized_rt_temp = std::make_shared<DenseDataRegion2D*>(new DenseDataRegion2D());
			(*this->normalized_rt_temp)->setName(normalized_rt_arg->getName());
			(*this->normalized_rt_temp)->setId(std::to_string(normalized_rt_arg->getId()));
			(*this->normalized_rt_temp)->setVersion(normalized_rt_arg->getId());
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
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;

	seg_nonoverlap = std::shared_ptr<cv::Mat>(new cv::Mat());
	
}

TaskSegmentation5::~TaskSegmentation5() {
}

bool TaskSegmentation5::run(int procType, int tid) {
	cv::Mat normalized_rt = (*this->normalized_rt_temp)->getData();


	cv::Mat seg_nonoverlap_temp;

	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "\t\t\tTaskSegmentation5 executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNucleiStg6(normalized_rt, minSizePl, watershedConnectivity, &*seg_open, &*seg_nonoverlap);
	
	uint64_t t2 = Util::ClockGetTimeProfile();


	std::cout << "\t\t\tTask Segmentation5 time elapsed: "<< t2-t1 << std::endl;
}

void TaskSegmentation5::updateDR(RegionTemplate* rt) {
	normalized_rt_temp = std::make_shared<DenseDataRegion2D*>(dynamic_cast<DenseDataRegion2D*>(
		rt->getDataRegion((*this->normalized_rt_temp)->getName(),
		(*this->normalized_rt_temp)->getId(), 0, stoi((*this->normalized_rt_temp)->getId()))));

}

void TaskSegmentation5::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "\t\t\t[TaskSegmentation5] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}

	this->seg_open = ((TaskSegmentation5*)t)->seg_open;

}

void TaskSegmentation5::resolveDependencies(ReusableTask* t) {
	// verify if the task type is compatible
	if (typeid(t) != typeid(TaskSegmentation6)) {
		std::cout << "\t\t\t[TaskSegmentation6] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
	}

	
	this->seg_open = ((TaskSegmentation4*)t)->seg_open;

}

bool TaskSegmentation5::reusable(ReusableTask* rt) {
	TaskSegmentation5* t = (TaskSegmentation5*)(rt);
	if (
		(*this->normalized_rt_temp)->getName() == (*t->normalized_rt_temp)->getName() &&
		this->minSizePl == t->minSizePl &&
		this->watershedConnectivity == t->watershedConnectivity &&

		true) {

		return true;
	} else {
		return false;
	}
	return true;
}

int TaskSegmentation5::size() {
	return 
		sizeof(int) + sizeof(int) +
		sizeof(int) + (*normalized_rt_temp)->getName().length()*sizeof(char) + sizeof(int) +
		sizeof(int) +
		sizeof(int) +

		0;
}

int TaskSegmentation5::serialize(char *buff) {
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
	int normalized_rt_id = stoi((*normalized_rt_temp)->getId());
	memcpy(buff+serialized_bytes, &normalized_rt_id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt name size
	int normalized_rt_name_size = (*normalized_rt_temp)->getName().length();
	memcpy(buff+serialized_bytes, &normalized_rt_name_size, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy normalized_rt name
	memcpy(buff+serialized_bytes, (*normalized_rt_temp)->getName().c_str(), normalized_rt_name_size*sizeof(char));
	serialized_bytes+=normalized_rt_name_size*sizeof(char);

	// copy field minSizePl
	memcpy(buff+serialized_bytes, &minSizePl, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy field watershedConnectivity
	memcpy(buff+serialized_bytes, &watershedConnectivity, sizeof(int));
	serialized_bytes+=sizeof(int);


	return serialized_bytes;
}

int TaskSegmentation5::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// create the normalized_rt
	this->normalized_rt_temp = std::make_shared<DenseDataRegion2D*>(new DenseDataRegion2D());

	// extract normalized_rt id
	int normalized_rt_id = ((int*)(buff+deserialized_bytes))[0];
	(*this->normalized_rt_temp)->setId(to_string(normalized_rt_id));
	(*this->normalized_rt_temp)->setVersion(normalized_rt_id);
	deserialized_bytes += sizeof(int);

	// extract normalized_rt name size
	int normalized_rt_name_size = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// copy normalized_rt name
	char normalized_rt_name[normalized_rt_name_size+1];
	normalized_rt_name[normalized_rt_name_size] = '\0';
	memcpy(normalized_rt_name, buff+deserialized_bytes, sizeof(char)*normalized_rt_name_size);
	deserialized_bytes += sizeof(char)*normalized_rt_name_size;
	(*this->normalized_rt_temp)->setName(normalized_rt_name);

	// extract field minSizePl
	this->minSizePl = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// extract field watershedConnectivity
	this->watershedConnectivity = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);


	return deserialized_bytes;
}

ReusableTask* TaskSegmentation5::clone() {
	ReusableTask* retValue = new TaskSegmentation5();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete[] buff;

	return retValue;
}

void TaskSegmentation5::print() {
	cout << "\t\t\tminSizePl: " << minSizePl << endl;
	cout << "\t\t\twatershedConnectivity: " << watershedConnectivity << endl;

}

// Create the task factory
ReusableTask* Segmentation5Factory1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new TaskSegmentation5(args, inputRt);
}

// Create the task factory
ReusableTask* Segmentation5Factory2() {
	return new TaskSegmentation5();
}

// register factory with the runtime system
bool registeredTaskSegmentation52 = ReusableTask::ReusableTaskFactory::taskRegister("TaskSegmentation5", 
	&Segmentation5Factory1, &Segmentation5Factory2);

TaskSegmentation6::TaskSegmentation6() {

}

TaskSegmentation6::TaskSegmentation6(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	
	int set_cout = 0;
	for(ArgumentBase* a : args){
		if (a->getName().compare("segmented_rt") == 0) {
			ArgumentRT* segmented_rt_arg;
			segmented_rt_arg = (ArgumentRT*)a;
			this->segmented_rt_temp = std::make_shared<DenseDataRegion2D*>(new DenseDataRegion2D());
			(*this->segmented_rt_temp)->setName(segmented_rt_arg->getName());
			(*this->segmented_rt_temp)->setId(std::to_string(segmented_rt_arg->getId()));
			(*this->segmented_rt_temp)->setVersion(segmented_rt_arg->getId());
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
	if (set_cout < args.size())
		std::cout << __FILE__ << ":" << __LINE__ <<" Missing common arguments on Segmentation" << std::endl;

	
}

TaskSegmentation6::~TaskSegmentation6() {
}

bool TaskSegmentation6::run(int procType, int tid) {

	cv::Mat segmented_rt;


	uint64_t t1 = Util::ClockGetTimeProfile();

	std::cout << "\t\t\tTaskSegmentation6 executing." << std::endl;	

	::nscale::HistologicalEntities::segmentNucleiStg7(&segmented_rt, minSizeSeg, maxSizeSeg, fillHolesConnectivity, &*seg_nonoverlap);
	
	uint64_t t2 = Util::ClockGetTimeProfile();

	(*this->segmented_rt_temp)->setData(segmented_rt);

	std::cout << "\t\t\tTask Segmentation6 time elapsed: "<< t2-t1 << std::endl;
}

void TaskSegmentation6::updateDR(RegionTemplate* rt) {
rt->insertDataRegion(*this->segmented_rt_temp);

}

void TaskSegmentation6::updateInterStageArgs(ReusableTask* t) {
	// verify if the tasks are compatible
	if (typeid(t) != typeid(this)) {
		std::cout << "\t\t\t[TaskSegmentation6] " << __FILE__ << ":" << __LINE__ <<" incompatible tasks." << std::endl;
		return;
	}

	this->seg_nonoverlap = ((TaskSegmentation6*)t)->seg_nonoverlap;

}

void TaskSegmentation6::resolveDependencies(ReusableTask* t) {
	// verify if the task type is compatible

	
	this->seg_nonoverlap = ((TaskSegmentation5*)t)->seg_nonoverlap;

}

bool TaskSegmentation6::reusable(ReusableTask* rt) {
	TaskSegmentation6* t = (TaskSegmentation6*)(rt);
	if (
		(*this->segmented_rt_temp)->getName() == (*t->segmented_rt_temp)->getName() &&
		this->minSizeSeg == t->minSizeSeg &&
		this->maxSizeSeg == t->maxSizeSeg &&
		this->fillHolesConnectivity == t->fillHolesConnectivity &&

		true) {

		return true;
	} else {
		return false;
	}
	return true;
}

int TaskSegmentation6::size() {
	return 
		sizeof(int) + sizeof(int) +
		sizeof(int) + (*segmented_rt_temp)->getName().length()*sizeof(char) + sizeof(int) +
		sizeof(int) +
		sizeof(int) +
		sizeof(int) +

		0;
}

int TaskSegmentation6::serialize(char *buff) {
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
	int segmented_rt_id = stoi((*segmented_rt_temp)->getId());
	memcpy(buff+serialized_bytes, &segmented_rt_id, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy segmented_rt name size
	int segmented_rt_name_size = (*segmented_rt_temp)->getName().length();
	memcpy(buff+serialized_bytes, &segmented_rt_name_size, sizeof(int));
	serialized_bytes+=sizeof(int);

	// copy segmented_rt name
	memcpy(buff+serialized_bytes, (*segmented_rt_temp)->getName().c_str(), segmented_rt_name_size*sizeof(char));
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

int TaskSegmentation6::deserialize(char *buff) {
	int deserialized_bytes = 0;

	// extract task id
	this->setId(((int*)(buff+deserialized_bytes))[0]);
	deserialized_bytes += sizeof(int);

	// extract parent task id
	this->parentTask = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// create the segmented_rt
	this->segmented_rt_temp = std::make_shared<DenseDataRegion2D*>(new DenseDataRegion2D());

	// extract segmented_rt id
	int segmented_rt_id = ((int*)(buff+deserialized_bytes))[0];
	(*this->segmented_rt_temp)->setId(to_string(segmented_rt_id));
	(*this->segmented_rt_temp)->setVersion(segmented_rt_id);
	deserialized_bytes += sizeof(int);

	// extract segmented_rt name size
	int segmented_rt_name_size = ((int*)(buff+deserialized_bytes))[0];
	deserialized_bytes += sizeof(int);

	// copy segmented_rt name
	char segmented_rt_name[segmented_rt_name_size+1];
	segmented_rt_name[segmented_rt_name_size] = '\0';
	memcpy(segmented_rt_name, buff+deserialized_bytes, sizeof(char)*segmented_rt_name_size);
	deserialized_bytes += sizeof(char)*segmented_rt_name_size;
	(*this->segmented_rt_temp)->setName(segmented_rt_name);

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

ReusableTask* TaskSegmentation6::clone() {
	ReusableTask* retValue = new TaskSegmentation6();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete[] buff;

	return retValue;
}

void TaskSegmentation6::print() {
	cout << "\t\t\tminSizeSeg: " << minSizeSeg << endl;
	cout << "\t\t\tmaxSizeSeg: " << maxSizeSeg << endl;
	cout << "\t\t\tfillHolesConnectivity: " << fillHolesConnectivity << endl;

}

// Create the task factory
ReusableTask* Segmentation6Factory1(list<ArgumentBase*> args, RegionTemplate* inputRt) {
	return new TaskSegmentation6(args, inputRt);
}

// Create the task factory
ReusableTask* Segmentation6Factory2() {
	return new TaskSegmentation6();
}

// register factory with the runtime system
bool registeredTaskSegmentation62 = ReusableTask::ReusableTaskFactory::taskRegister("TaskSegmentation6", 
	&Segmentation6Factory1, &Segmentation6Factory2);


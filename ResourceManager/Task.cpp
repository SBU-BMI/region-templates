/*
 * Task.cpp
 *
 *  Created on: Aug. 17, 2011
 *      Author: george
 */

#include "Task.h"
#include <stdio.h>
#include <exception>

int Task::instancesIdCounter = 1;
pthread_mutex_t Task::taskCreationLock = PTHREAD_MUTEX_INITIALIZER;

Task::Task()
{
	// Sets the speedup for each available processor to 1
	for(int i = 0; i < ExecEngineConstants::NUM_PROC_TYPES; i++){
		speedups[i] = 1.0;
	}

	// Lock to guarantee that identifier counter is not read or updated in parallel
	pthread_mutex_lock(&Task::taskCreationLock);

	// Sets current identifier to the task
	this->setId(Task::instancesIdCounter);

	// Increments the unique id
	Task::instancesIdCounter++;

	// Release lock
	pthread_mutex_unlock(&Task::taskCreationLock);

	// By default, tasks are created in the ACTIVE status
	this->setStatus(ExecEngineConstants::ACTIVE);

	// set the number of dependencies solved to be 0. Resource
	// Manager will modify this value according
	this->numberDependenciesSolved = 0;

	this->curExecEngine = NULL;
	this->setTaskType(ExecEngineConstants::PROC_TASK);
	this->setCallBackDepsReady(false);
}


Task::~Task()
{
//	std::cout << "~TASKID:" << this->getId()<< std::endl;
	try{
		if(curExecEngine != NULL){
			this->curExecEngine->resolveDependencies(this);
		}

	}catch(...){
		printf("ERROR resolveDependencies\n");
	}


}

Task* Task::tryPreassignment(){
	Task* preassignedTask = NULL;
	if(curExecEngine != NULL){
		preassignedTask = this->curExecEngine->tryPreassignment(this);
	}
	return preassignedTask;
}
// Adds a single dependency to the current task
void Task::addDependency(int dependencyId)
{
	this->dependencies.push_back(dependencyId);
}

// Adds a number of dependencies to the current task
void Task::addDependencies(vector<int> dependenciesIds)
{
	for(int i = 0; i < dependenciesIds.size(); i++){
		this->dependencies.push_back(dependenciesIds[i]);
	}
}

// Prints task dependencies
void Task::printDependencies(void)
{
	std::cout << "Task.id="<<this->getId()<<" #deps="<< this->dependencies.size()<< " :";
	if(this->dependencies.size() > 0){
		for(int i = 0; i < this->dependencies.size(); i++){
			std::cout << this->dependencies[i] <<";";
		}
	}
	std::cout << std::endl;
}

int Task::getNumberDependencies()
{
	return this->dependencies.size();
}

int Task::getDependency(int index)
{
	int retValue = -1;
	if(index >= 0 && index < this->getNumberDependencies()){
		retValue = this->dependencies[index];
	}
	return retValue;
}

int Task::incrementDepenciesSolved()
{
	this->numberDependenciesSolved++;
	return this->numberDependenciesSolved;
}

int Task::getNumberDependenciesSolved() const
{
    return numberDependenciesSolved;
}

int Task::getTaskType() const
{
    return taskType;
}

bool Task::isCallBackDepsReady() const
{
    return callBackDepsReady;
}

void Task::addDependency(Task *dependency)
{
	try{
		this->dependencies.push_back(dependency->getId());
	}catch(exception &e){
		std::cout << __FILE__<<":"<< __LINE__<< ". Exception: failed to addDependency, input depency address="<<dependency<<std::endl;
		std::cout << e.what() << std::endl;
	}
}

void Task::addArgument(TaskArgument* argument) {
	this->taskArguments.push_back(argument);
}

int Task::getArgumentId(int index) {
	int id = -1;
	if(index >= 0 && index < this->taskArguments.size()){
		id = taskArguments[index]->getId();
	}
	return id;
}

int Task::uploadArgument(int index, cv::gpu::Stream& stream) {
	if(index >= 0 && index < this->taskArguments.size()){
		taskArguments[index]->upload(stream);
	}
	return 0;
}

int Task::downloadArgument(int index, cv::gpu::Stream& stream) {
	if(index >= 0 && index < this->taskArguments.size()){
		taskArguments[index]->download(stream);
	}
	return 0;
}

int Task::getNumberArguments() {
	return taskArguments.size();
}

TaskArgument* Task::getArgument(int index) {
	TaskArgument* retArg = NULL;
	if(index >= 0 && index < this->taskArguments.size()){
		retArg = taskArguments[index];
	}
	return retArg;
}

void Task::setCallBackDepsReady(bool callBackDepsReady)
{
    this->callBackDepsReady = callBackDepsReady;
}

void Task::setTaskType(int taskType)
{
    this->taskType = taskType;
}

int Task::getId() const
{
    return id;
}
void Task::setId(int id)
{
    this->id = id;
}

//void *Task::getGPUTempData(int tid){
//	void * returnDataPtr=NULL;
//	if(curExecEngine != NULL){
//		returnDataPtr = curExecEngine->getGPUTempData(tid);
//	}
//	return returnDataPtr;
//}

// Change a task estimated speedup, initially set to 1
void Task::setSpeedup(int procType, float speedup)
{
	speedups[procType-1] = speedup;
}

// Simply retrieves the current speedup value for the requested processor type
float Task::getSpeedup(int procType) const
{
	return speedups[procType-1];
}

// Dispatches a given task for execution with the current execution engine
int Task::insertTask(Task *task){
	int retValue=0;
	if(task != NULL && curExecEngine != NULL){
		curExecEngine->insertTask(task);
	}else{
		std::cout << "Failed to insert a new task!" << std::endl;
		retValue=1;
	}
	return retValue;
}

// Default task computing function, which should be rewritten
// by the descending Task class specialization.
bool Task::run(int procType, int tid)
{
	std::cout <<"Warning. The \"run\" function from the Task class is being executed. You should implement the run into the descendant task class!"<< std::endl;
	return true;
}


CallBackTaskBase::CallBackTaskBase() {
	this->setTaskType(ExecEngineConstants::TRANSACTION_TASK);

}

CallBackTaskBase::~CallBackTaskBase() {
}

bool CallBackTaskBase::run(int procType, int tid)
{
	std::cout <<"Warning. The \"run\" function from the CallBackTaskBase class is being executed. You should implement the run into the descendant CallBackTaskBase class!"<< std::endl;
	return true;
}






/*
 * TrackDependencies.cpp
 *
 *  Created on: Feb 27, 2012
 *      Author: george
 */

#include "TrackDependencies.h"

pthread_mutex_t TrackDependencies::dependencyMapLock = PTHREAD_MUTEX_INITIALIZER;

TrackDependencies::TrackDependencies() {
	this->countTasksPending = 0;
	this->transactionTask = NULL;
	this->inputIOTaskId = -1;

}

void TrackDependencies::checkDependencies(Task* task, TasksQueue* tq) {
	std::map<int, std::list<Task *> >::iterator dependencyMapIt;

	// Lock dependency map
	pthread_mutex_lock(&TrackDependencies::dependencyMapLock);

	if(task->getTaskType() == ExecEngineConstants::IO_TASK){
		this->setInputIoTaskId(task->getId());
	}else{
		if(this->getInputIoTaskId() != -1)
			task->addDependency(this->getInputIoTaskId());
	}
//	std::cout << "TaskID: " << task->getId() << " nDeps: "<<task->getNumberDependencies() << std::endl;
	for(int i = 0; i < task->getNumberDependencies(); i++){
		// Retrieve id of the ith dependency, and check whether it is on the map: in other words, if
		// it is still executing or waiting to execute
		dependencyMapIt = dependencyMap.find(task->getDependency(i));

		// Dependency was found in the map. Add current task to the list of task it should
		// notify about the end of execution
		if(dependencyMapIt != dependencyMap.end()){
			dependencyMapIt->second.push_back(task);
			dependencyMapIt->second.size();

		}else{
			task->incrementDepenciesSolved();
		}
	}
//	std::cout << "TaskID: " << task->getId() << " nDeps: "<<task->getNumberDependencies() << " depsSolved: "<< task->getNumberDependenciesSolved()<< std::endl;

	// Insert current task into the map of active tasks (those processing or pending due to dependencies)
	list<Task *> l;

	if(this->transactionTask != NULL){
		// Okay, add task as dependency of the transaction Task
//		l.push_back(this->transactionTask);
		this->transactionTask->addDependency(task->getId());
	}

	dependencyMap.insert(std::pair<int, list<Task *> >(task->getId(), l));

	// Check whether all dependencies were solved, and dispatches task for execution if affirmative
	if(task->getNumberDependencies() == task->getNumberDependenciesSolved()){
		// It always starts empty, and tasks are added as they are dispatched for execution
		tq->insertTask(task);
		// std::cout << "[dependency_test] stage " << task->getId() << " is solved: " 
		// 	<< task->getNumberDependenciesSolved() << " out of "
		// 	<< task->getNumberDependencies() << std::endl;
	}else{
		this->incrementCountTasksPending();
		// std::cout << "[dependency_test] stage " << task->getId() << " is pending with " 
		// 	<< task->getNumberDependenciesSolved() << " out of "
		// 	<< task->getNumberDependencies() << " deps solved" << std::endl;
	}

	// Unlock dependency map
	pthread_mutex_unlock(&TrackDependencies::dependencyMapLock);

}

int TrackDependencies::getInputIoTaskId() const {
	return inputIOTaskId;
}

void TrackDependencies::setInputIoTaskId(int inputIoTaskId) {
	inputIOTaskId = inputIoTaskId;
}

Task* TrackDependencies::tryPreAssignement(Task* task, TasksQueue* tq,
		int schedType) {
	// By default, no task is preassigned
	Task* preAssignedTask = NULL;
	map<int, list<Task *> >::iterator dependencyMapIt;

	// Lock dependency map
	pthread_mutex_lock(&TrackDependencies::dependencyMapLock);

	// return data about task that has just finished
	dependencyMapIt = dependencyMap.find(task->getId());
	if(dependencyMapIt != dependencyMap.end()){
		// Get the number of tasks depending on this one.
		// Warning: Should not move the (dependencyMapIt->second.size()) to the loop (eliminating depsToSolve),
		// because the size of depencyMapIt->second is modified within the loop
		int depsToSolve = dependencyMapIt->second.size();

//		std::cout << "depsToSolve = "<< depsToSolve <<std::endl;

		list<Task *>::iterator dependentsListIt ;

		// for each task registered as dependency of the current task, resolve this dependency
		for(dependentsListIt = dependencyMapIt->second.begin(); dependentsListIt != dependencyMapIt->second.end(); dependentsListIt++){
			// takes pointer to the first dependent task
			Task *dependentTask = *dependentsListIt;

//			std::cout << "Task.NumDependencies = "<< dependentTask->getNumberDependencies()<< " depsSolved = "<< dependentTask->getNumberDependenciesSolved() <<std::endl;

			// if "task", which we just executed, is the last dependency, dependentTask is a potential task for pre-assignment
			if(dependentTask->getNumberDependenciesSolved() == (dependentTask->getNumberDependencies()-1)){
				if(schedType == ExecEngineConstants::FCFS_QUEUE && dependentTask->getTaskType() == ExecEngineConstants::PROC_TASK){
					// Increments counter regarding to the number of dependencies solved for the dependent task
					dependentTask->incrementDepenciesSolved();

					// decrement number of tasks pending with this trackDependencies objects
					this->decrementCountTasksPending();

					preAssignedTask = dependentTask;
					break;
				}else{
					// Priority
					if(schedType == ExecEngineConstants::PRIORITY_QUEUE && dependentTask->getTaskType() == ExecEngineConstants::PROC_TASK){
						// Current speedup of the (front, back) tasks with lower and higher speedups in the queue of tasks ready to execute.
						float front_speedup, back_speedup;
						tq->getFrontBackSpeedup(front_speedup, back_speedup);
//						std::cout << "Preassignment: front="<< front_speedup << " back="<< back_speedup<< " tq.size="<< tq->getSize() << " dependent.speedup="<< dependentTask->getSpeedup(ExecEngineConstants::GPU) <<std::endl;
						// there is no other tasks queued for execution, performing the preassignment
						if(front_speedup == -1 && back_speedup == -1){
//							std::cout << "Assignement1" << std::endl;
							// Increments counter regarding to the number of dependencies solved for the dependent task
							dependentTask->incrementDepenciesSolved();

							// decrement number of tasks pending with this trackDependencies objects
							this->decrementCountTasksPending();

							preAssignedTask = dependentTask;
							break;
						}else{
							float dependentTaskSpeedup = dependentTask->getSpeedup(ExecEngineConstants::GPU);
							//if(dependentTask->getSpeedup(ExecEngineConstants::GPU) > back_speedup )
							if(back_speedup - dependentTaskSpeedup < back_speedup *0.2  || dependentTaskSpeedup >= back_speedup){
//								std::cout << "Assignement2" << std::endl;
								// Increments counter regarding to the number of dependencies solved for the dependent task
								dependentTask->incrementDepenciesSolved();

								// decrement number of tasks pending with this trackDependencies objects
								this->decrementCountTasksPending();

								preAssignedTask = dependentTask;
								break;
							}
						}
					}

				}
			}

		}
	}
	// Unlock dependency map
	pthread_mutex_unlock(&TrackDependencies::dependencyMapLock);

	return preAssignedTask;
}

void TrackDependencies::resolveDependencies(Task* task, TasksQueue* tq) {
	map<int, list<Task *> >::iterator dependencyMapIt;

//	cout << "Resolve deps id: "<< task->getId() << " status: "<< task->getStatus() <<std::endl;

	// Lock dependency map
	pthread_mutex_lock(&TrackDependencies::dependencyMapLock);

	// return information about task that has just finished
	dependencyMapIt = dependencyMap.find(task->getId());
	if(dependencyMapIt != dependencyMap.end()){

		// Get the number of tasks depending on this one.
		// Warning: Should not move the (dependencyMapIt->second.size()) to the loop (eliminating depsToSolve),
		// because the size of depencyMapIt->second is modified within the loop
		int depsToSolve = dependencyMapIt->second.size();

//		std::cout << "depsToSolve: "<< depsToSolve << std::endl;
		// for each task registered as dependency of the current task, resolve this dependency
		for(int i = 0; i < depsToSolve;i++){
			// takes pointer to the first dependent task
			Task *dependentTask = dependencyMapIt->second.front();

			// removes tasks from the list of dependent tasks
			dependencyMapIt->second.pop_front();

			// Increments counter regarding to the number of dependencies solved for the dependent task
			dependentTask->incrementDepenciesSolved();

			// if current task is not in ACTIVE, modify dependent tasks to that status
			if(task->getStatus() != ExecEngineConstants::ACTIVE){
				dependentTask->setStatus(task->getStatus());
			}

//			std::cout << "\t\tDep task id: "<< dependentTask->getId() << " deps solved: "<< dependentTask->getNumberDependenciesSolved() << " #deps: "<<dependentTask->getNumberDependencies()<< std::endl;

			// if all dependencies of this tasks were solved, dispatches it to execution
			if(dependentTask->getNumberDependenciesSolved() == dependentTask->getNumberDependencies()){
//				if(dependentTask->getTaskType() == ExecEngineConstants::PROC_TASK){
					tq->insertTask(dependentTask);
					this->decrementCountTasksPending();
//				}else{
					// If all dependencies are solved, and endTransaction was called (meaning that isCallBackDesReady will return true)
//					if(dependentTask->getTaskType() == ExecEngineConstants::TRANSACTION_TASK && dependentTask->isCallBackDepsReady()){
//						dependentTask->run();
//						delete dependentTask;
//					}
//				}

			}
		}
		// Remove task that just finished from the dependency map
		dependencyMap.erase(dependencyMapIt);

	}else{
		std::cout << "Warning: task.id="<< task->getId() << "finished execution, but its data is not available at the dependencyMap" <<std::endl;
	}

	// Unlock dependency map
	pthread_mutex_unlock(&TrackDependencies::dependencyMapLock);
}

int TrackDependencies::getCountTasksPending() const {
	return this->countTasksPending;
}

void TrackDependencies::incrementCountTasksPending() {
	this->countTasksPending++;
}

void TrackDependencies::decrementCountTasksPending() {
	this->countTasksPending--;
}

TrackDependencies::~TrackDependencies() {

}

void TrackDependencies::lock() {
	pthread_mutex_lock(&TrackDependencies::dependencyMapLock);
}

void TrackDependencies::unlock() {
	pthread_mutex_unlock(&TrackDependencies::dependencyMapLock);
}

void TrackDependencies::startTransaction(CallBackTaskBase *transactionTask)
{
	if(this->transactionTask != NULL){
		std::cout << "Error: calling startTranscation before ending previous transaction (endTransaction)" <<std::endl;
	}
	this->transactionTask = transactionTask;
}



void TrackDependencies::endTransaction()
{
	if(this->transactionTask == NULL){
		std::cout << "Error: calling endTransaction before starting a transaction (startTranscation)" <<std::endl;
	}//else{

		// Lock dependency map
//		this->lock();

		// if all dependencies were solved before the program executes endTransaction
//		if(this->transactionTask->getNumberDependencies() == this->transactionTask->getNumberDependenciesSolved()){
			// All dependencies were solved, so execute callback function
//			this->transactionTask->run();
//		}
//		this->transactionTask->setCallBackDepsReady(true);

		// Unlock dependency map
//		this->unlock();
//	}
	this->setInputIoTaskId(-1);
	this->transactionTask = NULL;
}

std::list<Task *>  TrackDependencies::retrieveDependentTasks(int taskId) {
	map<int, list<Task *> >::iterator dependencyMapIt;
	std::list<Task *> dependentTasks;

	pthread_mutex_lock(&TrackDependencies::dependencyMapLock);

	dependencyMapIt = dependencyMap.find(taskId);
	if(dependencyMapIt != dependencyMap.end()){
		dependentTasks = dependencyMapIt->second;
	}

	pthread_mutex_unlock(&TrackDependencies::dependencyMapLock);
	return dependentTasks;
}

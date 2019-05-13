/*
 * ExecutionEngine.cpp
 *
 *  Created on: Aug 17, 2011
 *      Author: george
 */

#include "ExecutionEngine.h"
//pthread_mutex_t ExecutionEngine::dependencyMapLock = PTHREAD_MUTEX_INITIALIZER;

ExecutionEngine::ExecutionEngine(int cpuThreads, int gpuThreads, int queueType, bool dataLocalityAware, bool prefetching) {
	schedType = queueType;
	if(queueType ==ExecEngineConstants::FCFS_QUEUE){
		tasksQueue = new TasksQueueFCFS(cpuThreads, gpuThreads);
	}else{
		tasksQueue = new TasksQueuePriority(cpuThreads, gpuThreads);
	}
	threadPool = new ThreadPool(tasksQueue, this);
	threadPool->createThreadPool(cpuThreads, NULL, gpuThreads, NULL, dataLocalityAware, prefetching);

	this->trackDependencies = new TrackDependencies();
}

ExecutionEngine::~ExecutionEngine() {
	delete threadPool;
	delete tasksQueue;
	delete trackDependencies;

#ifdef USE_DISTRIBUTED_TILLING_EXAMPLE
	for (std::pair<std::string, openslide_t*> p : svsPointersCache) {
		openslide_close(p.second);
	}
#endif

}

//void *ExecutionEngine::getGPUTempData(int tid){
//	return threadPool->getGPUTempData(tid);
//}

bool ExecutionEngine::insertTask(Task *task)
{
	task->curExecEngine = this;

	// Resolve task dependencies and queue it for execution, or left the task pending waiting
	this->trackDependencies->checkDependencies(task, this->tasksQueue);

	return true;
}


Task *ExecutionEngine::getTask(int procType)
{
	return tasksQueue->getTask(procType);
}

void ExecutionEngine::startupExecution()
{
	threadPool->initExecution();
}

void ExecutionEngine::endExecution()
{
	// this protection is used just in case the user calls this function multiple times.
	// It will avoid a segmentation fault
	if(threadPool != NULL){
		tasksQueue->releaseThreads(threadPool->getGPUThreads() + threadPool->getCPUThreads());
		delete threadPool;
	}
	threadPool = NULL;
}

void ExecutionEngine::resolveDependencies(Task *task){
	// forward message to track dependencies class
	this->trackDependencies->resolveDependencies(task, this->tasksQueue);
}

Task* ExecutionEngine::tryPreassignment(Task *task){
	// forward message to track dependencies class
	return this->trackDependencies->tryPreAssignement(task, this->tasksQueue, this->schedType);
}

int ExecutionEngine::getCountTasksPending() const
{
    return this->trackDependencies->getCountTasksPending();
}


void ExecutionEngine::waitUntilMinQueuedTask(int numberQueuedTasks)
{
	if(numberQueuedTasks < 0) numberQueuedTasks = 0;

	// Loop waiting the number of tasks queued decrease
	while(numberQueuedTasks < tasksQueue->getSize()){
		usleep(100000);
	}

}

void ExecutionEngine::startTransaction(CallBackTaskBase *transactionTask)
{
	this->trackDependencies->startTransaction(transactionTask);
//	if(this->transactionTask != NULL){
//		std::cout << "Error: calling startTranscation before ending previous transaction (endTransaction)" <<std::endl;
//	}
//	this->transactionTask = transactionTask;
}



void ExecutionEngine::endTransaction()
{
	this->trackDependencies->endTransaction();
}

#ifdef USE_DISTRIBUTED_TILLING_EXAMPLE
openslide_t* ExecutionEngine::getSvsPointer(std::string path) {
	std::map<std::string, openslide_t*>::iterator it = svsPointersCache.find(path);
	openslide_t* osr;

	// If the pointer wasn't on cache, opens it
	if (it == svsPointersCache.end()) {
		osr = openslide_open(path.c_str());
		svsPointersCache[path] = osr;
		// std::cout << "[ExecutionEngine] New svs: " << path << std::endl;
	} else {
		osr = it->second;
		// std::cout << "[ExecutionEngine] Existing svs: " << path << std::endl;
	}
	return osr;
}
#endif


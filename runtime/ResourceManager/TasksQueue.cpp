/*
 * TasksQueue.cpp
 *
 *  Created on: Aug 17, 2011
 *      Author: george
 */

#include "TasksQueue.h"


TasksQueue::TasksQueue() {
	pthread_mutex_init(&queueLock, NULL);
	sem_init(&tasksToBeProcessed, 0, 0);
}

TasksQueue::~TasksQueue() {
	pthread_mutex_destroy(&queueLock);
	sem_destroy(&tasksToBeProcessed);
}
bool TasksQueue::insertTask(Task *task)
{
	return true;
}

Task *TasksQueue::getTask(int procType)
{
	return NULL;
}

Task* TasksQueue::getByTaskId(int id) {
	return NULL;
}

void TasksQueue::incrementTasksToProcess()
{
	sem_post(&tasksToBeProcessed);
}

int TasksQueue::getSize()
{
	return 0;
}


bool TasksQueueFCFS::insertTask(Task *task)
{
	pthread_mutex_lock(&queueLock);

	if(task->getTaskType() ==  ExecEngineConstants::RT_CACHE_DUMP){
		tasksQueue.push_front(task);
	}else{
		tasksQueue.push_back(task);
	}

	pthread_mutex_unlock(&queueLock);
	sem_post(&tasksToBeProcessed);
	return true;
}

Task *TasksQueueFCFS::getTask(int procType)
{
	Task *retTask = NULL;
	sem_wait(&tasksToBeProcessed);
	pthread_mutex_lock(&queueLock);

	if(tasksQueue.size() > 0){
		retTask = tasksQueue.front();
//		tasksQueue.pop_front();
#ifdef	LOAD_BALANCING
		if(ExecEngineConstants::CPU == procType){
			float taskSpeedup = retTask->getSpeedup(ExecEngineConstants::GPU);
			if( this->gpuThreads* taskSpeedup > tasksQueue.size()){
				retTask = NULL;
			}
		}
#endif
		if(retTask != NULL)
			tasksQueue.pop_front();
	}
	pthread_mutex_unlock(&queueLock);
	return retTask;
}

int TasksQueueFCFS::getSize()
{
	int number_tasks = 0;
	pthread_mutex_lock(&queueLock);

	number_tasks = tasksQueue.size();

	pthread_mutex_unlock(&queueLock);

	return number_tasks;
}


Task* TasksQueueFCFS::getByTaskId(int id) {
	list<Task*>::iterator it;
	Task* retValue = NULL;
	pthread_mutex_lock(&queueLock);

	for(it = tasksQueue.begin(); it != tasksQueue.end(); it++){
		// if found, get pointer value, erase it from list, and stop searching.
		if((*it)->getId() == id){
			retValue = (*it);
			tasksQueue.erase(it);
			break;
		}
	}
	pthread_mutex_unlock(&queueLock);
	return retValue;
}

bool TasksQueuePriority::insertTask(Task *task)
{
	pthread_mutex_lock(&queueLock);

	float taskSpeedup = task->getSpeedup(ExecEngineConstants::GPU);
	tasksQueue.insert(pair<float,Task*>(taskSpeedup, task));

	pthread_mutex_unlock(&queueLock);
	sem_post(&tasksToBeProcessed);
	return true;
}
Task *TasksQueuePriority::getTask(int procType)
{
	Task *retTask = NULL;
	sem_wait(&tasksToBeProcessed);
	pthread_mutex_lock(&queueLock);

	int taskQueueSize = tasksQueue.size();
//	std::cout << "TasksQueuePriority::getTask. QueueSize="<< taskQueueSize << std::endl;

	if(taskQueueSize > 0){
		multimap<float, Task*>::iterator it;

		if(procType == ExecEngineConstants::GPU){
			it = tasksQueue.end();
			it--;
			retTask = (*it).second;
			tasksQueue.erase(it);
		}else{
			it = tasksQueue.begin();
			retTask = (*it).second;
#ifdef	LOAD_BALANCING
			float taskSpeedup = retTask->getSpeedup(ExecEngineConstants::GPU);
			printf("Balancing on\n");
			if( this->gpuThreads*taskSpeedup > taskQueueSize){
				retTask = NULL;
			}else{
#endif
				tasksQueue.erase(it);
#ifdef	LOAD_BALANCING
			}
#endif
		}
	}
	pthread_mutex_unlock(&queueLock);
	return retTask;
}

int TasksQueuePriority::getSize()
{
	int number_tasks = 0;
	pthread_mutex_lock(&queueLock);

	number_tasks = tasksQueue.size();

	pthread_mutex_unlock(&queueLock);

	return number_tasks;
}
void TasksQueuePriority::getFrontBackSpeedup(float& front, float& back) {
	front = -1.0;
	back = -1.0;

	pthread_mutex_lock(&queueLock);

	if(tasksQueue.size() > 0){
			multimap<float, Task*>::iterator it;
			it = tasksQueue.begin();
			front = (*it).second->getSpeedup(ExecEngineConstants::GPU);
			it = tasksQueue.end();
			it--;

			back = (*it).second->getSpeedup(ExecEngineConstants::GPU);
	}
	pthread_mutex_unlock(&queueLock);
	return;
}

/*Task *TasksQueuePriority::getTask(int procType)
{
	Task *retTask = NULL;
	sem_wait(&tasksToBeProcessed);
	pthread_mutex_lock(&queueLock);

	if(tasksQueue.size() > 0){
		multimap<float, Task*>::iterator it;

		if(procType == ExecEngineConstants::GPU){
			it = tasksQueue.end();
			it--;
			retTask = (*it).second;
			tasksQueue.erase(it);
		}else{
			it = tasksQueue.begin();
			retTask = (*it).second;
			tasksQueue.erase(it);
		}
	}
	pthread_mutex_unlock(&queueLock);
	return retTask;
}*/

void TasksQueue::releaseThreads(int numThreads)
{
	// Increment the number of tasks to be processed according to the
	// number of threads accessing this queue. So, all them will get
	// a NULL task, what is interpreted as an end of work.
	for(int i = 0; i < numThreads; i++){
		sem_post(&tasksToBeProcessed);
	}
}

/*****************************************************************************/
/******************************* Halide Queue ********************************/
/*****************************************************************************/

bool TasksQueueHalide::insertTask(Task *task) {
	pthread_mutex_lock(&queueLock);

	// Add the task reference to the queue
	this->allTasksQueue[task->getId()] = task;

	// Add add the task id to each targets' list while checking if this task is
	// executable, i.e., there is at least a single thread for any of the task's
	// targets.
	bool executable = false;
	std::cout << "[TasksQueueHalide] Inserting task " << task->getId() << " into queue with targets ";
	for (int target : task->getTaskTargets()) {
		this->tasksPerTarget[target].push_back(task->getId());
		std::cout << target << ", ";
		if (this->threadsPerTarget[target] > 0)
			executable = true;
	}
	std::cout << std::endl;

	// Exits the execution if task is not executable
	if (!executable) {
		std::cout << "[TasksQueueHalide::insertTask] " 
			<< "Task " << task->getId() << " is not executable "
			<< "since there are no threads for any of its targets."
			<< std::endl;
		exit(-1);
	}

	pthread_mutex_unlock(&queueLock);
	sem_post(&tasksToBeProcessed);
	return true;
}

Task *TasksQueueHalide::getTask(int target) {
	Task *retTask = NULL;
	sem_wait(&tasksToBeProcessed);
	pthread_mutex_lock(&queueLock);

#ifdef DEBUG
	std::cout << "[TasksQueueHalide] Getting task of target " << target << std::endl;
#endif

	// Only returns a task if there is one
	if(this->allTasksQueue.size() > 0){
		// Gets the iterator for the id of the first task for the input target
		std::list<int>::iterator taskI 
			= this->tasksPerTarget[target].begin();

		// Only returns a task if there is one implementation for the input target
		if (taskI != this->tasksPerTarget[target].end()) {
			retTask = this->allTasksQueue[*taskI];
			this->allTasksQueue.erase(*taskI);
			this->tasksPerTarget[target].erase(taskI);
		}
	}
	pthread_mutex_unlock(&queueLock);

#ifdef DEBUG
	std::cout << "[TasksQueueHalide] Returning tasks " << retTask << std::endl;
#endif

	return retTask;
}

// Returns an executable task given the available resources
// Returns NULL if there are no executable tasks
Task* TasksQueueHalide::getTask(int availableCpus, int availableGpus) {
	Task *retTask = NULL;
	sem_wait(&tasksToBeProcessed);
	pthread_mutex_lock(&queueLock);

	std::list<int>::iterator taskI;

	// Gives priority for gpu tasks
	if (availableGpus>0) {
		taskI = this->tasksPerTarget[ExecEngineConstants::GPU].begin();
		if (taskI != this->tasksPerTarget[ExecEngineConstants::GPU].end()) {
			retTask = this->allTasksQueue[*taskI];
			this->allTasksQueue.erase(*taskI);
			this->tasksPerTarget[ExecEngineConstants::GPU].erase(taskI);
		}
	}

	// Attempt to get a cpu task if (i) there were no available gpu threads
	// or (ii) there were no gpu tasks on the queue
	if (availableGpus>0 && retTask==NULL) {
		taskI = this->tasksPerTarget[ExecEngineConstants::CPU].begin();
		if (taskI != this->tasksPerTarget[ExecEngineConstants::CPU].end()) {
			retTask = this->allTasksQueue[*taskI];
			this->allTasksQueue.erase(*taskI);
			this->tasksPerTarget[ExecEngineConstants::CPU].erase(taskI);
		}
	}

	pthread_mutex_unlock(&queueLock);
	return retTask;
}

int TasksQueueHalide::getSize() {
	int number_tasks = 0;
	pthread_mutex_lock(&queueLock);

	number_tasks = allTasksQueue.size();

	pthread_mutex_unlock(&queueLock);

	return number_tasks;
}


Task* TasksQueueHalide::getByTaskId(int id) {
	list<Task*>::iterator it;
	Task* retValue = NULL;
	pthread_mutex_lock(&queueLock);

	std::map<int, Task*>::iterator taskI = this->allTasksQueue.find(id);
	retValue = taskI->second;
	this->allTasksQueue.erase(taskI);
	
	pthread_mutex_unlock(&queueLock);
	return retValue;
}
/*
 * ExecutionEngine.h
 *
 *  Created on: Aug 17, 2011
 *      Author: george
 */

#ifndef EXECUTIONENGINE_H_
#define EXECUTIONENGINE_H_

#include <map>

#include "ThreadPool.h"
#include "TasksQueue.h"
#include "TrackDependencies.h"
#include "ExecEngineConstants.h"

#ifdef USE_DISTRIBUTED_TILLING_EXAMPLE
#include "openslide.h"
#endif

//#define FCFS_QUEUE	1
//#define PRIORITY_QUEUE	2
class Task;
class CallBackTaskBase;
class TasksQueue;
class ThreadPool;
class TrackDependencies;

class ExecutionEngine {

private:
	// Queue of tasks used ready to execute within this execution engine
	TasksQueue *tasksQueue;

	int schedType;
	bool dataLocalityAware;

	// Pool of threads that are consuming tasks from the associated queue of tasks
	ThreadPool *threadPool;

#ifdef USE_DISTRIBUTED_TILLING_EXAMPLE
	// Cache of openslide file pointers to avoid multiple opening of files
	std::map<std::string, openslide_t*> svsPointersCache;
#endif

	TrackDependencies *trackDependencies;

//	// This structure maps the id of a given tasks to
//	// those tasks which the execution depends on it.
//	std::map<int, std::list<Task *> > dependencyMap;
//
//	// Lock used to guarantee atomic access to the dependentyMap
//	static pthread_mutex_t dependencyMapLock;
//
//	// Verifies if task dependencies have been solved and queue it for execution,
//	// or leave the task as pending until dependencies are solved
//	void checkDependencies(Task* task);
//
//	// Resolve state of any task dependent on this one, this is called
//	// after the task has finished its execution
	void resolveDependencies(Task* task);

	// Check if the argument task "task" will dispatch the execution of any other dependent task,
	// in that case try to perform a pre-assignement and take advantage of the data location to
	//avoid unnecessary CPU-GPU data transfers
	Task* tryPreassignment(Task *task);
//
//	// This variable counts the number of tasks dispatched for execution with the
//	// execution engine that still pending waiting for a dependency to finish
//	int countTasksPending;
//
//	// Simply return the value of counting regading to number of tasks pending
    int getCountTasksPending() const;
//
//    // Increment and decrement the number of tasks pending
//	void incrementCountTasksPending();
//	void decrementCountTasksPending();

	// Retrieves the next task available for execution.
	Task* getTask(int procType=ExecEngineConstants::CPU);

	// Grants access to these classes, they need to access private function calls
	friend class Task;
	friend class ThreadPool;

public:
	ExecutionEngine(int cpuThreads, int gpuThreads, int queueType=ExecEngineConstants::FCFS_QUEUE, bool dataLocalityAware=false, bool prefetching=false);
	virtual ~ExecutionEngine();

	// Dispatches a given task for execution within the Resource Manager
	bool insertTask(Task* task);

//	void *getGPUTempData(int tid);

	// Execution engine will start computation of tasks
	void startupExecution();

	// No more tasks will be queued, and whenever the tasks
	// already queued are computed the execution engine will finish.
	// This is a blocking call.
	void endExecution();

	// Calling process will wait until the number of tasks queue
	// for execution is equal or smaller than "number_queue_tasks"
	void waitUntilMinQueuedTask(int number_queued_tasks);

	// This is used to associated one task (transactionTask) as dependent
	// of all other created within the calls of start and end transaction
	void startTransaction(CallBackTaskBase *transactionTask);
	void endTransaction();

#ifdef USE_DISTRIBUTED_TILLING_EXAMPLE
	// Retrieves the file pointer of a specific svs file. If the
	// file was not yet opened, opens it and adds the pointer to 
	// the svsPointersCache
	openslide_t* getSvsPointer(std::string path);
#endif

};

#endif /* EXECUTIONENGINE_H_ */

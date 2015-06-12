/*
 * TrackDependencies.h
 *
 *  Created on: Feb 27, 2012
 *      Author: george
 */

#ifndef TRACKDEPENDENCIES_H_
#define TRACKDEPENDENCIES_H_

#include <pthread.h>
#include "Task.h"
#include "TasksQueue.h"
#include "ExecEngineConstants.h"


class Task;
class TasksQueue;
class CallBackTaskBase;

class TrackDependencies {
private:

	// This structure maps the id of a given tasks to
	// those tasks which the execution depends on it.
	std::map<int, std::list<Task *> > dependencyMap;

	// Lock used to guarantee atomic access to the dependentyMap
	static pthread_mutex_t dependencyMapLock;

	// This variable counts the number of tasks dispatched for execution with the
	// execution engine that still pending waiting for a dependency to finish
	int countTasksPending;

	// The transaction task that should be associated with all other created during
	// a given interval defined by the startTransction and endTransaction calls
	CallBackTaskBase *transactionTask;

	// This is a task that is lunched to perform I/O operations, and all other
	// tasks created in the pipeline will depend on it.
	int inputIOTaskId;

    // Increment and decrement the number of tasks pending
	void incrementCountTasksPending();
	void decrementCountTasksPending();

public:
	TrackDependencies();
	virtual ~TrackDependencies();

	// Verifies if task dependencies have been solved and queue it for execution,
	// or leave the task as pending until dependencies are solved
	void checkDependencies(Task* task, TasksQueue* tq);

	// Resolve state of any task dependent on this one, this is called
	// after the task has finished its execution
	void resolveDependencies(Task* task, TasksQueue* tq);

	Task* tryPreAssignement(Task* task, TasksQueue* tq, int schedType = ExecEngineConstants::FCFS_QUEUE);

	// Simply return the value of counting regarding to number of tasks pending
    int getCountTasksPending() const;

    // return the id of tasks that depend on a given task.
    std::list<Task *> retrieveDependentTasks(int taskId);


    // This is used to associated one task (transactionTask) as dependent
	// of all other created within the calls of start and end transaction
	void startTransaction(CallBackTaskBase *transactionTask);
	void endTransaction();

    void lock();
    void unlock();
	int getInputIoTaskId() const;
	void setInputIoTaskId(int inputIoTaskId);
};

#endif /* TRACKDEPENDENCIES_H_ */

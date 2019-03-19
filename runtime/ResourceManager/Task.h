/*
 * Task.h
 *
 *  Created on: Aug 17, 2011
 *      Author: george
 */

#ifndef TASK_H_
#define TASK_H_

#include <unistd.h>
#include <stddef.h>
#include <iostream>
#include <vector>

#include "ExecutionEngine.h"
#include "ExecEngineConstants.h"
#include "TaskArgument.h"

#include "opencv2/cudaarithm.hpp" // new opencv 3.4.1

class ExecutionEngine;
class Task;

class Task {
	private:
		// Estimated speedup of the current task to each available processor.
		float speedups[ExecEngineConstants::NUM_PROC_TYPES];

		// Unique identifier of the class instance.
		int id;

		// Status of the tasks: ACTIVE (default); CANCELED (will not be executed)
		int status;

		// Auxiliary class variable used to assign an unique id to each class object instance.
		static int instancesIdCounter;

		// Lock used to guarantee atomicity in the task creation/id generation
		static pthread_mutex_t taskCreationLock;

		vector<TaskArgument*> taskArguments;

		// Contains the tasks that this current is dependent. In other words, the set of tasks
		// that should execute before the current task is dispatch for execution.
		vector<int> dependencies;

		// Number of dependencies solved: number of tasks that this task depends, and have
		// 								  already finished the execution.
		int numberDependenciesSolved;

		// Is this a processing tasks, the default, or a transaction task -  only used to
		// assert that a set of tasks were finalized before the computation continues?
		int taskType;

		// Grants access to this class for execution engine.
		friend class ExecutionEngine;

		// Pointer to the Execution Engine that is responsible for this task.
		ExecutionEngine *curExecEngine;

		// Simple setter for the task id
		void setId(int id);

		// This bool variable says whether the all dependencies that were to be associated to
		// this task have already being associated. It is only used by the CallBackTaskClass.
		// However, it has to be defined here because we can't typecast the Task pointer available
		// during the execution to an object of type CallBackTask.
		bool callBackDepsReady;

	public:

		Task();
		virtual ~Task();

		void addArgument(TaskArgument *argument);
		TaskArgument* getArgument(int index);
		int getNumberArguments();
		int getArgumentId(int index);
		int uploadArgument(int index, cv::cuda::Stream& stream);
		int downloadArgument(int index, cv::cuda::Stream& stream);

		Task* tryPreassignment();

		// Adds a single dependency.
		void addDependency(int dependencyId);

		// Add a dependency to current task. The input is the task that the current depends on
		void addDependency(Task *dependency);

		// Adds a number of dependencies at once.
		void addDependencies(vector<int> dependenciesIds);

		int getNumberDependenciesSolved() const;

		// Retrieves the number of dependencies for this task
		int getNumberDependencies();

		// Retrieves id of a given task on dependencies
		int getDependency(int index);

		// Called to increment the variable that keeps record of the number of dependencies solved.
		// The return is the value of dependencies solved after incrementing it.
		int incrementDepenciesSolved();

		// Simple function used for debugging purposes
		void printDependencies(void);

		// Assigns a speedup to this task for a given processor architecture.
		void setSpeedup(int procType=ExecEngineConstants::GPU, float speedup=1.0);

		// Retrieves the expected speedup of this task for a given processor.
		float getSpeedup(int procType=ExecEngineConstants::GPU) const;

		// Inserts a new task to the current Execution Engine. The one executing this task.
		int insertTask(Task *task);
		//	void *getGPUTempData(int tid);

		// Interface implemented by the end user
		virtual bool run(int procType=ExecEngineConstants::GPU, int tid=0);

		// Other simple getters and setters
		int getId() const;
		int getTaskType() const;
		void setTaskType(int taskType);
		bool isCallBackDepsReady() const;
		void setCallBackDepsReady(bool callBackDepsReady);

		int getStatus() const {
			return status;
		}

		void setStatus(int status) {
			this->status = status;
		}

};

class CallBackTaskBase: public Task {
	public:
		CallBackTaskBase();
		virtual ~CallBackTaskBase();

		virtual bool run(int procType=ExecEngineConstants::GPU, int tid=0);
};


#endif /* TASK_H */

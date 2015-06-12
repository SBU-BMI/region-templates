

#include <stdio.h>
#include <sys/time.h>
#include "TaskId.h"
#include "ExecutionEngine.h"

#define NUM_TASKS	6

int main(int argc, char **argv){
	
	ExecutionEngine *execEngine = new ExecutionEngine(2, 1, ExecEngineConstants::PRIORITY_QUEUE);

	int nextTaskDependency;


	// Creates first task, which does not have dependencies
	TaskId *ts = new TaskId();
	ts->setSpeedup(ExecEngineConstants::GPU, 1.0);

	// Gets Id of the current task to set as dependency of the following
	nextTaskDependency = ts->getId();

	// Create a second task without dependencies
	TaskId *ts1 = new TaskId();
	int seconTaskId = ts1->getId();

	// Dispatches current tasks for execution
	execEngine->insertTask(ts);


	// Create the following tasks as a pipeline
	for(int i = 0; i < NUM_TASKS-1; i++){
		// Create empty task
		TaskId *ts = new TaskId();

		// Set GPU speedup
		ts->setSpeedup(ExecEngineConstants::GPU, 2.0);

		// Associates dependency for task
		ts->addDependency(nextTaskDependency);
		if(i==0){
			ts->addDependency(seconTaskId);
			execEngine->insertTask(ts);
			execEngine->insertTask(ts1);
			nextTaskDependency = ts->getId();
		}else{

			// Updates the dependency to be used in the next tasks in the pipeline
			nextTaskDependency = ts->getId();

			// Dispatches current task for execution
			execEngine->insertTask(ts);
		}
	}
	// Computing threads startup consuming tasks
	execEngine->startupExecution();

	// No more task will be assigned for execution. Waits
	// until all currently assigned have finished.
	execEngine->endExecution();
	
	delete execEngine;
	return 0;
}



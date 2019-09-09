#ifndef EXEC_ENGINE_CONSTANTS_H_
#define EXEC_ENGINE_CONSTANTS_H_

using namespace std;

namespace ExecEngineConstants {
	//! Defining what processor should be used when invoking the functions
	static const int NUM_PROC_TYPES=3;
	static const int CPU=1;
	static const int GPU=2;
	static const int MIC=3;
	// static const int PROC_TYPES[NUM_PROC_TYPES] = {CPU, GPU, MIC};

	//! Scheduling policies
	static const int FCFS_QUEUE=1;
	static const int PRIORITY_QUEUE=2;
	static const int DATA_LOCALITY_AWARE=3;
	static const int HALIDE_TARGET_QUEUE=4;

	//! Type of task assigned to execution. Most of tasks are computing tasks,
	// while there is a special type of tasks called transaction tasks that is used
	// o assert that all tasks created within a given interval were finalized
	// before some action is taken
	static const int PROC_TASK=1;
	static const int TRANSACTION_TASK=2;
	static const int IO_TASK=3;
	static const int RT_CACHE_DUMP=4;

	// Type of data sent to a task
	static const int INPUT = 1;
	static const int OUTPUT = 2;
	static const int INPUT_OUTPUT = 3;

	// Task status
	static const int ACTIVE = 1; // Means that everything is as expected and it should be processed
	static const int CANCELED = 2; // something else in the pipeline was canceled, and this task should be also
};

#endif

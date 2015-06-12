#ifndef TASK_COMPUTE_H_
#define TASK_COMPUTE_H_

#include "Task.h"

class TaskCompute: public Task {
private:
	int cpu_time;
	int gpu_time;
	int mic_time;
	std::string name;
	
public:
	TaskCompute(int cpu_time, int gpu_time, int mic_time, std::string name);

	virtual ~TaskCompute();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif

#ifndef TASK_MULTIPLY_H_
#define TASK_MULTIPLY_H_

#include "Task.h"

class TaskMultiply: public Task {
private:
	int *a;
	int *b;
	int *c;
public:
	TaskMultiply(int* a, int* b, int* c);

	virtual ~TaskMultiply();

	// a = b * c
	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_MULTIPLY_H_ */

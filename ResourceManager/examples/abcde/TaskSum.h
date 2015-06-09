

#ifndef TASK_SUM_H_
#define TASK_SUM_H_

#include "Task.h"

class TaskSum: public Task {
private:
	int *a;
	int *b;
	int *c;
public:
	TaskSum(int* a, int* b, int* c);

	virtual ~TaskSum();

	// a = b + c
	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_SUM_H_ */

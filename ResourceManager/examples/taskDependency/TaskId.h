

#ifndef TASKID_H_
#define TASKID_H_

#include "Task.h"

class TaskId: public Task {
private:

public:
	TaskId();

	virtual ~TaskId();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASKID_H_ */

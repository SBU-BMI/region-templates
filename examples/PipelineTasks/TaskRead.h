#ifndef TASK_READ_H_
#define TASK_READ_H_

#include "Task.h"
#include "opencv2/opencv.hpp"

class TaskRead: public Task {
private:
	std::string file_name;
	
public:
	TaskRead(std::string file_name);

	virtual ~TaskRead();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif

/*
 * TaskDoAll.h
 *
 *  Created on: Mar 3, 2012
 *      Author: george
 */

#ifndef TASKDOALL_H_
#define TASKDOALL_H_

#include "Task.h"
#include "opencv2/opencv.hpp"
#include "opencv2/gpu/gpu.hpp"
#include "HistologicalEntities.h"
#include "PixelOperations.h"
#include "MorphologicOperations.h"
#include "Util.h"
#include "ObjFeatures.h"

class TaskDoAll:public Task {
private:
	std::string inputFileName;

public:
	TaskDoAll(std::string inputFileName);
	~TaskDoAll();

	// Perform computation of the nuclei candidates
	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASKDOALL_H_ */

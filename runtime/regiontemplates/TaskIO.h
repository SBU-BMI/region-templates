/*
 * TaskIO.h
 *
 *  Created on: Feb 21, 2013
 *      Author: george
 */

#ifndef TASKIO_H_
#define TASKIO_H_

#include "Task.h"
#include "RTPipelineComponentBase.h"

class RTPipelineComponentBase;

class TaskIO : public Task{
private:
	// this is a pointer to the RTPipeline comment to which this task will execute the I/O operations
	RTPipelineComponentBase *associatedComponent;

	TaskIO();
public:
	TaskIO(RTPipelineComponentBase *associatedComponent);
	virtual ~TaskIO();

	//
	bool run(int procType=ExecEngineConstants::CPU, int tid=0);

};

#endif /* TASKIO_H_ */

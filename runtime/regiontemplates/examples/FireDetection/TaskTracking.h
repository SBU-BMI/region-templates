#ifndef TASK_TRACKING_H_
#define TASK_TRACKING_H_

#include "Task.h"
#include "DenseDataRegion2D.h"
#include "Util.h"
#include "testVideo.h"

class TaskTracking: public Task {
private:
//	DenseDataRegion2D* bgr;
//	DenseDataRegion2D* mask;
	// Id of the Firetracking component that created this task
	int componentId;

public:
	TaskTracking(int componentId);

	virtual ~TaskTracking();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
	int getComponentId() const;
	void setComponentId(int componentId);
};

#endif /* TASK_TRACKING_H_ */

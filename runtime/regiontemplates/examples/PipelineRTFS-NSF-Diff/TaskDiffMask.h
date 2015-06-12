

#ifndef TASK_DIFF_MASK_H_
#define TASK_DIFF_MASK_H_

#include "Task.h"
#include "DenseDataRegion2D.h"
#include "opencv2/opencv.hpp"


#include "Util.h"

class TaskDiffMask: public Task {
private:
	DenseDataRegion2D* mask0;
	DenseDataRegion2D* mask1;
	DenseDataRegion2D* feat0;
	DenseDataRegion2D* feat1;

public:
	TaskDiffMask(DenseDataRegion2D* mask0, DenseDataRegion2D* mask1, DenseDataRegion2D* feat0, DenseDataRegion2D* feat1);

	virtual ~TaskDiffMask();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_DIFF_MASK_H_ */

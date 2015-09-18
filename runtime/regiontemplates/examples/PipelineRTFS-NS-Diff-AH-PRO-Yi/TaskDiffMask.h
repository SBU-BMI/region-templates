

#ifndef TASK_DIFF_MASK_H_
#define TASK_DIFF_MASK_H_

#include "Task.h"
#include "DenseDataRegion2D.h"
#include "opencv2/opencv.hpp"


#include "Util.h"

class TaskDiffMask: public Task {
private:
	DenseDataRegion2D* dr1;
	DenseDataRegion2D* dr2;

	// | #diffPixels | #foreGroundPixelReferenceImage |
	int *diffPixels;

public:
	TaskDiffMask(DenseDataRegion2D* dr1, DenseDataRegion2D* dr2, int *diffPercentage);

	virtual ~TaskDiffMask();

	bool run(int procType=ExecEngineConstants::CPU, int tid=0);
};

#endif /* TASK_DIFF_MASK_H_ */

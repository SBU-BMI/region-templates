

#ifndef TASK_DIFF_MASK_H_
#define TASK_DIFF_MASK_H_

#include "Task.h"
#include "DenseDataRegion2D.h"
#include "opencv2/opencv.hpp"


#include "Util.h"

class TaskDiffMask: public Task {
protected:
	DenseDataRegion2D* dr1;
	DenseDataRegion2D* dr2;
    int *diff;

public:
	virtual ~TaskDiffMask();

//    void setDr1(DenseDataRegion2D *dr1);
//    void setDr2(DenseDataRegion2D *dr2);
//    void setDiff(int *diff);
    virtual bool run(int procType = ExecEngineConstants::CPU, int tid = 0) = 0;

};

#endif /* TASK_DIFF_MASK_H_ */


#ifndef DIFF_MASK_COMP_H_
#define DIFF_MASK_COMP_H_

#include "RTPipelineComponentBase.h"
//#include "../examples/PipelineRTFS-NS-Diff-AH/TaskDiffMask.h"

class DiffMaskComp : public RTPipelineComponentBase {
private:
// Percentage of difference found in the mask
//	float diffPercentage;

    //TaskDiffMask* task;

public:
	DiffMaskComp();
	virtual ~DiffMaskComp();
	int run();
    //void setTask(TaskDiffMask* task);

};

#endif

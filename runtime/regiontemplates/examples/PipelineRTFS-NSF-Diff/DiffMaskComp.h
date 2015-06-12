
#ifndef DIFF_MASK_COMP_H_
#define DIFF_MASK_COMP_H_

#include "TaskDiffMask.h"
#include "RTPipelineComponentBase.h"


class DiffMaskComp : public RTPipelineComponentBase {
private:


public:
	DiffMaskComp();
	virtual ~DiffMaskComp();

	int run();

};

#endif

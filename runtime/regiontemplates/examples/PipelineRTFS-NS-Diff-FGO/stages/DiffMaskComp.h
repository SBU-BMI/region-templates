
#ifndef DIFF_MASK_COMP_H_
#define DIFF_MASK_COMP_H_

#include "RTPipelineComponentBase.h"
#include <string>

class DiffMaskComp : public RTPipelineComponentBase {
private:
// Percentage of difference found in the mask
//	float diffPercentage;

    //TaskDiffMask* task;

    // data region id
	// IMPORTANT: this need to be set during the creation of this object
	int dr_id;

public:
	DiffMaskComp();
	virtual ~DiffMaskComp();
	int run();
    //void setTask(TaskDiffMask* task);

    void set_dr_id(int id) {dr_id = id;};

};

#endif

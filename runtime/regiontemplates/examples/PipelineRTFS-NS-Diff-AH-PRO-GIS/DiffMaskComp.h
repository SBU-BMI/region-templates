#ifndef DIFF_MASK_COMP_H_
#define DIFF_MASK_COMP_H_

#include "RTPipelineComponentBase.h"


class DiffMaskComp : public RTPipelineComponentBase {
private:
    // Percentage of difference found in the mask
//	float diffPercentage;

public:
    DiffMaskComp();

    virtual ~DiffMaskComp();

    int run();

};

#endif

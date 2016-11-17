#ifndef DICE_MASK_COMP_H_
#define DICE_MASK_COMP_H_

#include "RTPipelineComponentBase.h"


class DiceMaskComp : public RTPipelineComponentBase {
private:
    // Percentage of difference found in the mask
//	float diffPercentage;

public:
    DiceMaskComp();

    virtual ~DiceMaskComp();

    int run();

};

#endif

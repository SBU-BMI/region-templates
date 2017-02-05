#ifndef DICE_NOT_COOL_MASK_COMP_H_
#define DICE_NOT_COOL_MASK_COMP_H_

#include "RTPipelineComponentBase.h"


class DiceNotCoolMaskComp : public RTPipelineComponentBase {
private:
    // Percentage of difference found in the mask
//	float diffPercentage;

public:
    DiceNotCoolMaskComp();

    virtual ~DiceNotCoolMaskComp();

    int run();

};

#endif

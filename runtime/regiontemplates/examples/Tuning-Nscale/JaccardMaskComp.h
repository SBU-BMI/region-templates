#ifndef JACCARD_MASK_COMP_H_
#define JACCARD_MASK_COMP_H_

#include "RTPipelineComponentBase.h"


class JaccardMaskComp : public RTPipelineComponentBase {
private:
    // Percentage of difference found in the mask
//	float diffPercentage;

public:
    JaccardMaskComp();

    virtual ~JaccardMaskComp();

    int run();

};

#endif

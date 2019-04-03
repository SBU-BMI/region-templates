#ifndef DIFF_MASK_COMP_H_
#define DIFF_MASK_COMP_H_

#include "RTPipelineComponentBase.h"
#include "regiontemplates/comparativeanalysis/pixelcompare/PixelCompare.h"
#include "stagesIds.h"

class DiffMaskComp : public RTPipelineComponentBase {
private:
// Percentage of difference found in the mask
//  float diffPercentage;

    //TaskDiffMask* task;

public:
    DiffMaskComp();
    virtual ~DiffMaskComp();

    // Add the required RT and DR to the stage
    void setIo(std::string inRtName, std::string maskRtName, 
        std::string ddrName);
    
    int run();
    //void setTask(TaskDiffMask* task);

};

#endif

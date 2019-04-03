#ifndef NORMALIZATION_COMP_H_
#define NORMALIZATION_COMP_H_

#include "opencv2/opencv.hpp"
// #include "opencv2/gpu/gpu.hpp" // old opencv 2.4
#include "opencv2/cudaarithm.hpp" // new opencv 3.4.1

#include "RTPipelineComponentBase.h"
#include "Util.h"
#include "FileUtils.h"

#include "stagesIds.h"
#include "TaskNormalization.h"

class NormalizationComp : public RTPipelineComponentBase {
private:
    cv::Mat inputImage;
    cv::Mat bgr;

public:
    NormalizationComp();
    virtual ~NormalizationComp();

    // Add the required RT and DR to the stage
    void setIo(std::string rtName, std::string ddrName);

    int run();
};

#endif /* NORMALIZATION_COMP_H_ */

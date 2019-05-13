/*
 * Segmentation.h
 *
 *  Created on: Feb 12, 2013
 *  Author: george
 */

#ifndef SEGMENTATION_H_
#define SEGMENTATION_H_

#include "opencv2/opencv.hpp"
// #include "opencv2/gpu/gpu.hpp" // old opencv 2.4
// #include "opencv2/cudaarithm.hpp" // new opencv 3.4.1

#include "RTPipelineComponentBase.h"
#include "Util.h"
#include "FileUtils.h"

#include "stagesIds.h"
#include "TaskSegmentation.h"

class Segmentation : public RTPipelineComponentBase {

public:
    Segmentation();
    virtual ~Segmentation();

    // Add the required RT and DR to the stage
    void setIo(std::string rtName, std::string ddrName);

    int run();

};

#endif /* SEGMENTATION_H_ */

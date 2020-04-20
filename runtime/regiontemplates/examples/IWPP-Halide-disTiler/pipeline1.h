#ifndef PIPELINE_1_H_
#define PIPELINE_1_H_

#include "misc.h"

#include "cv.hpp"
#include "Halide.h"

#include "RegionTemplate.h"
#include "ExecEngineConstants.h"
#include "Util.h"

#include "HalideIwpp.h"

// Should use ExecEngineConstants::GPU ... 
typedef int Target_t;

bool pipeline1(std::vector<cv::Mat>& im_ios, Target_t target, 
             std::vector<ArgumentBase*>& params);

#endif
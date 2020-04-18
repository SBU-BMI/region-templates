#ifndef HALIDE_IWPP_H_
#define HALIDE_IWPP_H_ value

// #define WITH_CUDA

#include <iostream>

#include "cv.hpp"
#include "opencv2/core/cuda.hpp"
#ifdef WITH_CUDA
#include "opencv2/cudaarithm.hpp"
#endif // WITH_CUDA

#include "Halide.h"

#include "ExecEngineConstants.h"
#include "Util.h"
#include "misc.h"

// Should use ExecEngineConstants::GPU ... 
typedef int Target_t;

template <typename T>
Halide::Func halSum(Halide::Buffer<T>& JJ);

extern int loopedIwppRecon(Target_t target, cv::Mat& cvHostI, cv::Mat& cvHostJ);

#endif

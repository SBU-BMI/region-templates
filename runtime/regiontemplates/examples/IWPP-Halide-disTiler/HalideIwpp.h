#ifndef HALIDE_IWPP_H_
#define HALIDE_IWPP_H_ value

#include <iostream>

#include "cv.hpp"
#include "opencv2/core/cuda.hpp"
#include "opencv2/cudaarithm.hpp"

#include "Halide.h"

#include "ExecEngineConstants.h"
#include "Util.h"

enum IwppExec {
    SERIAL,
    CPU,
    GPU
};

template <typename T>
Halide::Func halSum(Halide::Buffer<T>& JJ);

template <typename T>
extern int loopedIwppRecon(IwppExec exOpt, Halide::Buffer<T>& II, 
    Halide::Buffer<T>& JJ, Halide::Buffer<T>& hOut);

template <typename T>
extern int loopedIwppReconGPU(IwppExec exOpt, cv::cuda::GpuMat& cvDevI, 
    cv::cuda::GpuMat& cvDevJ, cv::Mat& cvHostOut);



#endif

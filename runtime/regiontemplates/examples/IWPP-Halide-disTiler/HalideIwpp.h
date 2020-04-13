#ifndef HALIDE_IWPP_H_
#define HALIDE_IWPP_H_ value

#define WITH_CUDA

#include <iostream>

#include "cv.hpp"
#include "opencv2/core/cuda.hpp"
#ifdef WITH_CUDA
#include "opencv2/cudaarithm.hpp"
#endif // WITH_CUDA

#include "Halide.h"

#include "ExecEngineConstants.h"
#include "Util.h"

enum IwppExec {
    CPU,
    CPU_REORDER,
    GPU,
    GPU_REORDER,
};

// Should use ExecEngineConstants::GPU ... 
typedef int Target_t;

template <typename T>
Halide::Func halSum(Halide::Buffer<T>& JJ);

template <typename T>
extern int loopedIwppRecon(Target_t target, IwppExec exOpt, 
    Halide::Buffer<T>& II, Halide::Buffer<T>& JJ,Halide::Buffer<T>& hOut, 
    cv::cuda::GpuMat& cvDevJ, cv::Mat& cvHostOut);

template <typename T>
extern int loopedIwppReconGPU(IwppExec exOpt, cv::cuda::GpuMat& cvDevI, 
    cv::cuda::GpuMat& cvDevJ, cv::Mat& cvHostOut);



#endif
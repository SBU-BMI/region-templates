#ifndef MISC_H_
#define MISC_H_

#include "cv.hpp"
#include "Halide.h"

#include "RegionTemplate.h"
#include "ExecEngineConstants.h"

RegionTemplate* newRT(std::string name, cv::Mat* data = NULL);

template <typename T>
Halide::Buffer<T> mat2buf(cv::Mat* m, std::string name="unnamed");

// This manual setup is required since GPU data may have a different
// memory displacement than on host memory. Tldr, need to fix
// the stride of Halide::Buffer for data already on GPU.
template <typename T>
Halide::Buffer<T> gpuMat2buf(cv::cuda::GpuMat& m, Halide::Target& t, 
        std::string name="");

#endif
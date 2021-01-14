#ifndef MISC_H_
#define MISC_H_

#define WITH_CUDA

#include "Halide.h"
#include "cv.hpp"

#include "ExecEngineConstants.h"
#include "RegionTemplate.h"

RegionTemplate *newRT(std::string name, cv::Mat *data = NULL);

template <typename T>
Halide::Buffer<T> mat2buf(cv::Mat *m, std::string name = "unnamed");

// This manual setup is required since GPU data may have a different
// memory displacement than on host memory. Tldr, need to fix
// the stride of Halide::Buffer for data already on GPU.
// OBS: somehow this cannot be placed on source file. If done,
// only gpuMat2buf will be "undefined reference" on "loopedIwppRecon"
#ifdef WITH_CUDA
template <typename T>
inline Halide::Buffer<T> gpuMat2buf(cv::cuda::GpuMat &m, Halide::Target &t,
                                    std::string name = "unnamed") {
    // buffer_t devB = {0, NULL, {m.cols, m.rows},
    //                  {1, ((int)m.step)/((int)sizeof(T))},
    //                  {0, 0}, sizeof(T)};

    buffer_t devB = {0, NULL, {m.cols, m.rows}, {1, m.cols}, {0, 0}, sizeof(T)};

    std::cout << "[" << name << "]: " << m.cols << "x" << m.rows << " - "
              << m.step << std::endl;

    Halide::Buffer<T> hDev =
        name.empty() ? Halide::Buffer<T>(devB) : Halide::Buffer<T>(devB, name);
    hDev.device_wrap_native(Halide::DeviceAPI::CUDA, (intptr_t)m.data, t);

    return hDev;
}
#endif  // if WITH_CUDA

#endif
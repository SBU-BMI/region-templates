#include <iostream>

#include "cv.hpp"
#include "opencv2/gpu/gpu.hpp"

#include "Halide.h"

int main(int argc, char const *argv[]) {
    
    // Create base img
    int w = 10;
    int h = 10;
    cv::Mat cvHostInput(h, w, CV_32S, cv::Scalar(0));
    cv::Mat cvHostOutput(h, w, CV_32S, cv::Scalar(0));
    cv::Mat cvHostRef(h, w, CV_32S, cv::Scalar(0));
    for (int i=0; i<h; i++) {
        for (int j=0; j<w; j++) {
            cvRef.at<int>(i,j) = i+j;
        }
    }
    
    // 0. opencv sum on host ==================================================
    std::cout << "[0] sum: " << cv::sum(cvRef)[0] << std::endl;

    // 1. opencv sum on dev ===================================================
    cv::gpu::GpuMat cvDevRef;
    cvDevRef.upload(cvHostRef);

    // Perform sum
    long gSum =  cv::cuda::sum(cvDevRef)[0];
    std::cout << "[1] cv sum on dev: " << gSum << std::endl;

    // 2. opencv sum after halide =============================================
    cv::gpu::GpuMat cvDevInput, cvDevOutput;
    cvDevInput.upload(cvHostInput);
    cvDevOutput.upload(cvHostInput);

    // set halide target
    Halide::Target target = Halide::get_host_target();
    target.set_feature(Halide::Target::CUDA);

    // Create Halide Buffers
    Halide::Buffer<int> hHostInput(cvHostInput.data, w, h, "hHostInput");
    Halide::Buffer<int> hHostOutput(cvHostOutput.data, w, h, "hHostOutput");
    Halide::Buffer<int> hDevInput(nullptr, w, h, "hDevInput");
    hDevInput.device_wrap_native(Halide::DeviceAPI::CUDA, (intptr_t)cvDevInput.data, target);
    Halide::Buffer<int> hDevOutput(nullptr, w, h, "hDevOutput");
    hDevOutput.device_wrap_native(Halide::DeviceAPI::CUDA, (intptr_t)cvDevOutput.data, target);

    // Halide pipelines
    Halide::Func fcpu, fgpu;
    Halide::Var x("x,"), y("y");

    fcpu(x,y) = hHostInput(x,y) + x + y;
    fgpu(x,y) = hDevInput(x,y) + x + y;

    fcpu.compile_jit();
    fgpu.compile_jit(target);

    fcpu.realize(hHostOutput);
    fgpu.realize(hDevOutput);

    std::cout << "[2] cpu sum: " << ((long)cv::sum(cvHostOutput)[0]) << std::endl;
    std::cout << "[2] gpu sum: " << ((long)cv::cuda::sum(cvDevOutput)[0]) << std::endl;
    
    return 0;
}

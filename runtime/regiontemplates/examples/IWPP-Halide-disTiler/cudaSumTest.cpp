#include <iostream>

#include "cv.hpp"
#include "opencv2/core/cuda.hpp"
#include "opencv2/cudaarithm.hpp"

#include "Halide.h"

int main(int argc, char const *argv[]) {
    
    // Create base img
    int w = 10;
    int h = 10;
    cv::Mat cvHostInput(cv::Size(h, w), CV_32SC1, cv::Scalar(0));//, cv::Mat::CONTINUOUS_FLAG);
    cv::Mat cvHostOutput(cv::Size(h, w), CV_32SC1, cv::Scalar(0));
    cv::Mat cvHostRef(cv::Size(h, w), CV_32SC1, cv::Scalar(0));
    for (int i=0; i<h; i++) {
        for (int j=0; j<w; j++) {
            cvHostRef.at<int>(i,j) = i;
        }
    }
    
    // 0. opencv sum on host ==================================================
    std::cout << "[0] sum: " << cv::sum(cvHostRef)[0] << std::endl;

    // 1. opencv sum on dev ===================================================
    cv::cuda::GpuMat cvDevRef;
    cvDevRef.upload(cvHostRef);

    // Perform sum
    long gSum =  cv::cuda::sum(cvDevRef)[0];
    std::cout << "[1] cv sum on dev: " << gSum << std::endl;

    // 2. opencv sum after halide =============================================
    cv::cuda::GpuMat cvDevInput, cvDevOutput;
    cvDevInput.upload(cvHostInput);
    cvDevOutput.upload(cvHostInput);

    // set halide target
    Halide::Target target = Halide::get_host_target();
    target.set_feature(Halide::Target::CUDA);

    // Create Halide Buffers
    Halide::Buffer<int> hHostInput((int*)cvHostInput.data, {w, h}, "hHostInput");
    Halide::Buffer<int> hHostOutput((int*)cvHostOutput.data, {w, h}, "hHostOutput");

    Halide::Buffer<int> hDevInput(nullptr, {w, h}, "hDevInput");
    hDevInput.device_wrap_native(Halide::DeviceAPI::CUDA, (intptr_t)cvDevInput.data, target);

    buffer_t hDevOutputB = {0, NULL, {w,h}, {1,cvDevOutput.step/sizeof(int)}, {0,0}, sizeof(int)};
    Halide::Buffer<int> hDevOutput(hDevOutputB, "hDevOutput");
    hDevOutput.device_wrap_native(Halide::DeviceAPI::CUDA, (intptr_t)cvDevOutput.data, target);

    // Halide pipelines
    Halide::Func fcpu("fcpu"), fgpu("fgpu");
    Halide::Var x("x"), y("y"), t, b;

    fcpu(x,y) = hHostInput(x,y) + x;
    fgpu(x,y) = hDevInput(x,y) + x;

    fcpu.compile_jit();
    fcpu.realize(hHostOutput);

    fgpu.gpu_tile(y, b, t, 1);
    fgpu.compile_jit(target);
    fgpu.compile_to_lowered_stmt("fgpu.html", {}, Halide::HTML, target);
    fgpu.realize(hDevOutput);

    std::cout << "[2] cpu sum: " << ((long)cv::sum(cvHostOutput)[0]) << std::endl;
    std::cout << "[2] gpu sum: " << ((long)cv::cuda::sum(cvDevOutput)[0]) << std::endl;
    std::cout << "[2] cvDevInput step: " << cvDevOutput.step << std::endl;
    std::cout << "[2] cvHostInput step: " << cvHostOutput.step << std::endl;
    return 0;
}

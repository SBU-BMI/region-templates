#include <iostream>

#include "cv.hpp"
#include "opencv2/gpu/gpu.hpp"

#include "Halide.h"

int main(int argc, char const *argv[]) {
    
    // 1. simple opencv sum ===================================================

    // Get input image
    cv::Mat input = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);

    // Create GPU data
    cv::gpu::GpuMat gpuData;
    gpuData.upload(input);

    // Perform sum
    long s =  cv::cuda::sum(gpuData)[0];

    std::cout << "sum: " << s << std::endl;

    // 2. halide exec with opencv gpu mat =====================================
    // Create GPU data
    cv::Mat input = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);
    cv::gpu::GpuMat cvDevData;
    cvDevData.upload(input);

    Halide::Buffer<uint8_t> hDevData(nullptr, cvDevData.cols, cvDevData.rows, "d");
    hDevData.device_wrap_native(Halide::halide_cuda_device_interface(), cvDevData.data);

    Halide::Func blurx("blurx");
    Halide::Var x("x,"), y("y"), bx, by, tx, ty;

    blurx(x,y) = hDevData(x,y)/2;

    // Schedule on gpu
    blurx.gpu_tile(x, y, bx, by, tx, ty, 16, 16);

    // compile for gpu
    Halide::Target target = Halide::get_host_target();
    target.set_feature(Halide::Target::CUDA);
    blurx.compile_jit(target);

    // Realize blur
    blurx.realize(hDevData);

    // Perform sum on gpu data
    long s =  cv::cuda::sum(cvDevData)[0];
    std::cout << "sum: " << s << std::endl;
    
    return 0;
}

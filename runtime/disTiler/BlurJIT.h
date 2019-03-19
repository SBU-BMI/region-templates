#ifndef BLUR_JIT_H
#define BLUR_JIT_H

#include "Halide.h"
#include <opencv2/opencv.hpp>

enum ret_t {
    SUCCESS
};

class BlurJIT {
public:
    BlurJIT(cv::Mat& input, cv::Mat& output);
    ret_t sched();
    ret_t run();
    cv::Mat getResult();

private:
    cv::Mat input;
    cv::Mat output;

    int rows, cols;

    // halide functions
    Halide::Func blurx;
    Halide::Func hf_output;
    Halide::Func clamped;

    Halide::Buffer<uint8_t> hb_input;
    Halide::Buffer<uint8_t> hb_output;

    Halide::Var x, y;
};

#endif

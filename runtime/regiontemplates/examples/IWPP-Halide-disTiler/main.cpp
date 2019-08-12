#include <iostream>
#include <stdio.h>

#include "cv.hpp"
#include "Halide.h"

// #include "AutoRegionTemplate.h"
// #include "AutoStage.h"
#include "IwppRecon.h"

using std::cout;
using std::endl;

extern "C" int loopedIwppRecon(halide_buffer_t* bI, halide_buffer_t* bJJ) {
    Halide::Buffer<uint8_t> I(*bI);
    Halide::Buffer<uint8_t> JJ(*bJJ);

    Halide::Func rasterx, rastery, arasterx, arastery;
    Halide::RDom se(-1,3,-1,3);
    Halide::Var x, y;

    // Clamping input
    Halide::Func J = Halide::BoundaryConditions::repeat_edge(JJ);

    int32_t w = bI->dim[0].extent;
    int32_t h = bI->dim[1].extent;

    // Clamping rasterx for rastery input
    Halide::Expr xc = clamp(x, 0, w-1);
    Halide::Expr yc = clamp(y, 0, h-1);
    Halide::Func rasterxc;

    // Functions for vertical and horizontal propagation
    rasterx(x,y) = min(I(x,y), maximum(J(x+se.x, y)));
    // arasterx(w-x,y) = min(I(w-x,y), maximum(rasterx(w-x+se.x, y)));
    rasterxc(x,y) = rasterx(xc, yc);
    rastery(x,y) = min(I(x,y), maximum(rasterxc(x, y+se.y)));
    // arastery(x,h-y) = min(I(x,h-y), maximum(rastery(x, h-y+se.y)));

    // Schedule
    rasterx.compute_root();
    Halide::Var xi, xo;
    rastery.reorder(y,x).serial(y);
    rastery.split(x, xo, xi, 16);
    rastery.vectorize(xi).parallel(xo);

    rastery.compile_jit();

    int oldSum = 0;
    int newSum = 0;
    int it = 0;

    // Create a cv::Mat wrapper for the halide pipeline output buffer
    cv::Mat cvJ(h, w, CV_8U, JJ.get()->raw_buffer()->host);
    do {
        it++;
        oldSum = newSum;
        rastery.realize(JJ);
        newSum = cv::sum(cvJ)[0];
        // cout << "new - old: " << newSum << " - " << oldSum << endl;
        // cv::imwrite("out.png", cvJ);
    } while(newSum != oldSum);

    return 0;
}

int main(int argc, char const *argv[]) {

    // Manages inputs
    if (argc != 3) {
        cout << "usage: ./iwpp <I image> <J image>" << endl;
        return 0;
    }
    cv::Mat* cvI = new cv::Mat(cv::imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE));
    cv::Mat* cvJ = new cv::Mat(cv::imread(argv[2], CV_LOAD_IMAGE_GRAYSCALE));
    
    // RTF::AutoRegionTemplate* rt;

    // rt = new AutoRegionTemplate(inputImgPath);

    Halide::Func halCpu;
    // Halide::Func halGpu;
    // Halide::Var x, y;
    
    // Same def for both cpu and gpu
    // halCpu(x,y) = ...
    // halGpu(x,y) = ...

    // IwppParallelRecon iwpp;
    // iwpp.realize(cvI, cvJ);
    Halide::Buffer<uint8_t> I(cvI->data, cvI->cols, cvI->rows);
    Halide::Buffer<uint8_t> J(cvJ->data, cvJ->cols, cvJ->rows);
    halCpu.define_extern("loopedIwppRecon", {I, J}, Halide::UInt(8), 2);
    cv::Mat cvOut(cvJ->cols, cvJ->rows, CV_8U);
    Halide::Buffer<uint8_t> out(cvOut.data, cvOut.cols, cvOut.rows);
    halCpu.realize(out);


    // Schedule funcs for both cpu and gpu
    // halCpu.parallel(x);
    // halGpu.gpu_tile(...);

    // Generate the RTF stage
    // RTF::AutoStage stage({RTF_DEV::CPU, halCpu}, {RTF_DEV::GPU, halGpu});
    // stage.setRT(rt);
    // stage.distribute(RTF_TILING_ALG::REGULAR);

    // Executes the task
    // stage.execute();

    // Gets the final output
    // cv::Mat output = rt.getResult();

    return 0;
}
#include <iostream>

#include "cv.hpp"
#include "Halide.h"

// #include "AutoRegionTemplate.h"
// #include "AutoStage.h"
#include "IwppRecon.h"

using std::cout;
using std::endl;

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

    int oldSum = 0;
    int newSum = 0;
    int it = 0;
    IwppParallelRecon iwpp;
    do {
        it++;
        oldSum = newSum;
        iwpp.realize(cvI, cvJ);
        newSum = cv::sum(*cvJ)[0];
        // cout << "new - old: " << newSum << " - " << oldSum << endl;
        // cv::imwrite("out.png", *cvJ);
    } while(newSum != oldSum);

    cout << "Done in " << it << " iterations" << endl;

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
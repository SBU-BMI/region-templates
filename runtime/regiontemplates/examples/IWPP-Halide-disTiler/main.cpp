#include <iostream>

#include "cv.hpp"
#include "Halide.h"
#include "halide_image_io.h"

// #include "AutoRegionTemplate.h"
// #include "AutoStage.h"

using std::cout;
using std::endl;

class IwppParallelRecon {
private:
    Halide::ImageParam I;
    Halide::ImageParam JJ;
    Halide::Param<int32_t> w;
    Halide::Param<int32_t> h;
    Halide::Func iwppFunc;

public:
    IwppParallelRecon() : I(Halide::type_of<uint8_t>(), 2), 
            JJ(Halide::type_of<uint8_t>(), 2) {

        Halide::Func rasterx, rastery, arasterx, arastery;
        Halide::RDom se(-1,3,-1,3);
        Halide::Var x, y;

        // Clamping input
        Halide::Func J = Halide::BoundaryConditions::repeat_edge(JJ);
        
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


        rastery.reorder(y,x).serial(y);
        Halide::Var xi, xo;
        rastery.split(x, xo, xi, 16);
        rastery.vectorize(xi).parallel(xo);
        rastery.store_root();

        iwppFunc = rastery;
        cout << "done generating" << endl;
        iwppFunc.compile_jit();
        cout << "done compiling" << endl;
    }

    int realize(cv::Mat* cvI, cv::Mat* cvJ) {
        int32_t cols = cvI->cols;
        int32_t rows = cvI->rows;
        
        // cout << "configuring" << endl;
        w.set(cols);
        h.set(rows);
        I.set(Halide::Buffer<uint8_t>(cvI->data, cols, rows));
        JJ.set(Halide::Buffer<uint8_t>(cvJ->data, cols, rows));

        Halide::Buffer<uint8_t> hOut(cvJ->data, cols, rows);

        // cout << "realizing" << endl;
        iwppFunc.realize(hOut);
    }
};

void swap(cv::Mat* m1, cv::Mat* m2) {
    cv::Mat* tmp = m1;
    m1 = m2;
    m2 = tmp;
}

int main(int argc, char const *argv[]) {

    if (argc != 3) {
        cout << "usage: ./iwpp <I image> <J image>" << endl;
        return 0;
    }

    cv::Mat* cvI = new cv::Mat(cv::imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE));
    cv::Mat* cvJ = new cv::Mat(cv::imread(argv[2], CV_LOAD_IMAGE_GRAYSCALE));
    // cv::Mat* cvJ2 = new cv::Mat(cv::imread(argv[2], CV_LOAD_IMAGE_GRAYSCALE));
    // cv::Mat* cvJ2 = new cv::Mat(cv::Size(cvI->cols, cvI->rows), CV_8U);
    
    // RTF::AutoRegionTemplate* rt;

    // rt = new AutoRegionTemplate(inputImgPath);

    Halide::Func halCpu;
    // Halide::Func halGpu;
    // Halide::Var x, y;
    
    // Same def for both cpu and gpu
    // halCpu(x,y) = ...
    // halGpu(x,y) = ...

    int sumAll = 0;
    int newSum = 0;
    // halCpu = makeRAR_IWPP(cvI, cvJ);
    IwppParallelRecon iwpp;
    do {
        // cout << "J2-J1: " << cv::sum((*cvJ2)-(*cvJ1))[0] << endl;
        sumAll = newSum;
        iwpp.realize(cvI, cvJ);
        newSum = cv::sum(*cvJ)[0];
        cout << "new - old: " << newSum << " - " << sumAll << endl;
        cv::imwrite("out.png", *cvJ);
        // swap(cvJ1, cvJ2);
    } while(newSum != sumAll);
    // } while(cv::sum((*cvJ2)-(*cvJ1))[0] > 0); 

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
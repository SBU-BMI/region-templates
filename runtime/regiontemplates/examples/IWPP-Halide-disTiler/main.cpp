#include <iostream>
#include <stdio.h>

#include "cv.hpp"
#include "Halide.h"

// #include "AutoRegionTemplate.h"
#include "AutoStage.h"
#include "IwppRecon.h"
#include "RegionTemplate.h"

using std::cout;
using std::endl;

// CPU sched still marginally better since data is copied to and from device
// on every propagation loop instance
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
    int sched = RTF::CPU;
    rasterx.compute_root();
    Halide::Target target = Halide::get_host_target();
    if (sched == RTF::CPU) {
        Halide::Var xi, xo;
        rastery.reorder(y,x).serial(y);
        rastery.split(x, xo, xi, 16);
        rastery.vectorize(xi).parallel(xo);
        rastery.compile_jit();
    } else if (sched == RTF::GPU) {
        Halide::Var bx, by, tx, ty;
        rasterx.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        rastery.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        target.set_feature(Halide::Target::CUDA);
        I.set_host_dirty();
        JJ.set_host_dirty();
    }
    // target.set_feature(Halide::Target::Debug);
    rastery.compile_jit(target);
    // rastery.compile_to_lowered_stmt("raster.html", {}, Halide::HTML);


    int oldSum = 0;
    int newSum = 0;
    int it = 0;

    // Create a cv::Mat wrapper for the halide pipeline output buffer
    cv::Mat cvJ(h, w, CV_8U, JJ.get()->raw_buffer()->host);
    do {
        it++;
        oldSum = newSum;
        rastery.realize(JJ);
        // Copy from GPU to host is done every realization which is inefficient.
        // However this is just done as a proof of concept for having a CPU 
        // and a GPU sched (more work necessary for running the sum on GPU). 
        if (sched == RTF::GPU) {
            JJ.copy_to_host();
        }
        newSum = cv::sum(cv::Mat(h, w, CV_8U, JJ.get()->raw_buffer()->host))[0];
        cout << "new - old: " << newSum << " - " << oldSum << endl;
        // cv::imwrite("out.png", cvJ);
    } while(newSum != oldSum);

    return 0;
}

// A wrapper of loopedIwppRecon with an explicit output buffer
extern "C" int loopedIwppRecon2(halide_buffer_t* bI, 
    halide_buffer_t* bJJ, halide_buffer_t* bOut) {
    cout << "hereeeeeeeeeeeeeeeeeeeeeee" << endl;
    loopedIwppRecon(bI, bJJ);
    Halide::Buffer<uint8_t> JJ(*bJJ);
    Halide::Buffer<uint8_t> Out(*bOut);
    Out.copy_from(JJ);
}

void extern_exec(cv::Mat* cvI, cv::Mat* cvJ) {
    // Create the buffers for execution
    Halide::Buffer<uint8_t> I(cvI->data, cvI->cols, cvI->rows);
    Halide::Buffer<uint8_t> J(cvJ->data, cvJ->cols, cvJ->rows);

    // Create extern halide func
    Halide::Func halCpu;
    halCpu.define_extern("loopedIwppRecon", {I, J}, Halide::UInt(8), 2);
    cv::Mat cvOut(cvJ->cols, cvJ->rows, CV_8U);
    
    // Realizes the pipeline to the output
    Halide::Buffer<uint8_t> out(cvOut.data, cvOut.cols, cvOut.rows);
    halCpu.realize(out);
}

RegionTemplate* newRT(std::string name, cv::Mat* data = NULL) {
    RegionTemplate* rt = new RegionTemplate();
    rt->setName(name);
    DataRegion *dr = new DenseDataRegion2D();
    dr->setName(name);
    if (data != NULL) {
        ((DenseDataRegion2D*)dr)->setIsAppInput(true);
        ((DenseDataRegion2D*)dr)->setInputFileName(name);
        ((DenseDataRegion2D*)dr)->setData(*data);  
    }
    rt->insertDataRegion(dr);
    return rt;
}

int main(int argc, char *argv[]) {

    // Manages inputs
    if (argc != 3) {
        cout << "usage: ./iwpp <I image> <J image>" << endl;
        return 0;
    }
    cv::Mat* cvI = new cv::Mat(cv::imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE));
    cv::Mat* cvJ = new cv::Mat(cv::imread(argv[2], CV_LOAD_IMAGE_GRAYSCALE));

    // =========== trying v0.3 === Using RTF for execution of halide pipeline
    // Creates the inputs
    RegionTemplate* rtI = newRT(argv[1], cvI);
    RegionTemplate* rtJ = newRT(argv[2], cvJ);
    RegionTemplate* rtOut = newRT("Out");

    // Create the halide stage
    struct : RTF::HalGen {
        RTF::Target_t getTarget() {return RTF::CPU;}
        void realize(const std::vector<cv::Mat*>& im_ios, 
                     const std::vector<int>& param_ios) {

            // Wraps the input and output cv::mat's with halide buffers
            Halide::Buffer<uint8_t> hI;// = mat2buf(im_ios[0]);
            Halide::Buffer<uint8_t> hJ;// = mat2buf(im_ios[1]);
            Halide::Buffer<uint8_t> hOut;// = mat2buf(im_ios[2]);
            
            // Define halide stage
            Halide::Func halCpu;
            halCpu.define_extern("loopedIwppRecon2", {hI, hJ, hOut}, 
                Halide::UInt(8), 2);

            // Adds the cpu implementation to the schedules output
            halCpu.realize(hOut);
        }
    } stage1_hal;

    cout << "[main] creating stage" << endl;

    RTF::AutoStage stage1({cvI->rows, cvI->cols}, {RTF::ASInputs<>(rtI->getName()), 
        RTF::ASInputs<>(rtJ->getName()), RTF::ASInputs<>(rtOut->getName())}, 
        &stage1_hal);
    cout << "[main] adding rts to stage" << endl;
    stage1.addRegionTemplateInstance(rtI, rtI->getName());
    stage1.addRegionTemplateInstance(rtJ, rtJ->getName());
    stage1.addRegionTemplateInstance(rtOut, rtOut->getName());
    stage1.execute(argc, argv);

    // =========== Working v0.2 === Halide external func complete with iteration
    // extern_exec(cvI, cvJ);

    // =========== Working v0.1 === Class with halide parameters
    // ============================ Iteration of propagation inside class
    // IwppParallelRecon iwpp;
    // iwpp.realize(cvI, cvJ);

    // =========== expected / goal
    // norm->addRegionTemplateInstance(rt, rt->getName());

    // RTF::AutoRegionTemplate* rt;

    // rt = new AutoRegionTemplate(inputImgPath);

    // Halide::Func halGpu;
    // Halide::Var x, y;
    
    // Same def for both cpu and gpu
    // halCpu(x,y) = ...
    // halGpu(x,y) = ...

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
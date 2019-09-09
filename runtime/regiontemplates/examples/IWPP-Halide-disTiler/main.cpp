#include <iostream>
#include <stdio.h>

#include "cv.hpp"
#include "Halide.h"

#include "AutoStage.h"
#include "IwppRecon.h"
#include "RegionTemplate.h"
#include "ExecEngineConstants.h"

#include "TiledRTCollection.h"
#include "IrregTiledRTCollection.h"
#include "costFuncs/BGMasker.h"
#include "costFuncs/ThresholdBGMasker.h"

using std::cout;
using std::endl;

// CPU sched still marginally better since data is copied to and from device
// on every propagation loop instance
extern "C" int loopedIwppRecon(halide_buffer_t* bI, halide_buffer_t* bJJ,
    int sched) {

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
    Halide::Target target = Halide::get_host_target();
    if (sched == ExecEngineConstants::CPU) {
        Halide::Var xi, xo;
        rastery.reorder(y,x).serial(y);
        rastery.split(x, xo, xi, 16);
        rastery.vectorize(xi).parallel(xo);
        rastery.compile_jit();
    } else if (sched == ExecEngineConstants::GPU) {
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
        if (sched == ExecEngineConstants::GPU) {
            JJ.copy_to_host();
        }
        newSum = cv::sum(cv::Mat(h, w, CV_8U, JJ.get()->raw_buffer()->host))[0];
        // cout << "new - old: " << newSum << " - " << oldSum << endl;
        // cv::imwrite("out.png", cvJ);
    } while(newSum != oldSum);

    return 0;
}

// A wrapper of loopedIwppRecon with an explicit output buffer
extern "C" int loopedIwppRecon2(halide_buffer_t* bI, 
    halide_buffer_t* bJJ, halide_buffer_t* bOut, int sched) {
    // cout << "hereeeeeeeeeeeeeeeeeeeeeee" << endl;
    loopedIwppRecon(bI, bJJ, sched);
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
    if (data != NULL) {
        DataRegion *dr = new DenseDataRegion2D();
        dr->setName(name);
        ((DenseDataRegion2D*)dr)->setData(*data);  
        rt->insertDataRegion(dr);
    }
    return rt;
}

template <typename T>
Halide::Buffer<T> mat2buf(cv::Mat* m) {
    return Halide::Buffer<uint8_t>(m->data, m->cols, m->rows);
}

// Needs to be static for referencing across mpi processes/nodes
static struct : RTF::HalGen {
    std::string getName() {return "stage1_cpu";}
    int getTarget() {return ExecEngineConstants::CPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

        // Wraps the input and output cv::mat's with halide buffers
        cv::Mat grayI;
        cv::cvtColor(im_ios[0], grayI, CV_BGR2GRAY);
        Halide::Buffer<uint8_t> hI = mat2buf<uint8_t>(&grayI);
        Halide::Buffer<uint8_t> hJ = mat2buf<uint8_t>(&im_ios[1]);
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[2]);

        // Define halide stage
        Halide::Func halCpu;
        halCpu.define_extern("loopedIwppRecon2", {hI, hJ, hOut, 
            ExecEngineConstants::CPU}, Halide::UInt(8), 2);

        // Adds the cpu implementation to the schedules output
        cout << "[stage1][cpu] Realizing..." << endl;
        halCpu.realize(hOut);
        cout << "[stage1][cpu] Done..." << endl;
    }
} stage1_cpu;

// Needs to be static for referencing across mph processes/nodes
static struct : RTF::HalGen {
    std::string getName() {return "stage1_gpu";}
    int getTarget() {return ExecEngineConstants::GPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

        // Wraps the input and output cv::mat's with halide buffers
        cv::Mat grayI;
        cv::cvtColor(im_ios[0], grayI, CV_BGR2GRAY);
        Halide::Buffer<uint8_t> hI = mat2buf<uint8_t>(&grayI);
        Halide::Buffer<uint8_t> hJ = mat2buf<uint8_t>(&im_ios[1]);
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[2]);
        
        // Define halide stage
        Halide::Func halGpu;
        halGpu.define_extern("loopedIwppRecon2", {hI, hJ, hOut, 
            ExecEngineConstants::GPU}, Halide::UInt(8), 2);

        // Adds the cpu implementation to the schedules output
        cout << "[stage1][gpu] Realizing..." << endl;
        halGpu.realize(hOut);
        cout << "[stage1][gpu] Done..." << endl;
    }
} stage1_gpu;

// Needs to be static for referencing across mpi processes/nodes
static struct : RTF::HalGen {
    std::string getName() {return "stage2_cpu";}
    int getTarget() {return ExecEngineConstants::CPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0]);
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1]);
        
        // Define halide stage
        Halide::Func blurx, blury;
        Halide::Var x, y;
        Halide::RDom se(-1,3,-1,3);

        // Generate a bounded hIn for accessing out of border values
        Halide::Func hbIn = Halide::BoundaryConditions::repeat_edge(hIn);

        // Implement blur
        blurx(x,y) = sum(hbIn(x+se.x,y))/3;
        blury(x,y) = sum(blurx(x,y+se.y))/3;

        // Perform scheduling
        blurx.compute_root().parallel(y);
        blury.compute_root().parallel(x);

        // Adds the cpu implementation to the schedules output
        cout << "[stage2][cpu] Realizing..." << endl;
        blury.realize(hOut);
        cout << "[stage2][cpu] Done..." << endl;
    }
} stage2_cpu;

// Needs to be static for referencing across mpi processes/nodes
static struct : RTF::HalGen {
    std::string getName() {return "stage2_gpu";}
    int getTarget() {return ExecEngineConstants::GPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0]);
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1]);
        
        // Define halide stage
        Halide::Func blurx, blury;
        Halide::Var x, y;
        Halide::RDom se(-1,3,-1,3);

        // Generate a bounded hIn for accessing out of border values
        Halide::Func hbIn = Halide::BoundaryConditions::repeat_edge(hIn);

        // Implement blur
        blurx(x,y) = sum(hbIn(x+se.x,y))/3;
        blury(x,y) = sum(blurx(x,y+se.y))/3;

        // Perform scheduling
        blurx.compute_root().parallel(y);
        blury.compute_root().parallel(x);

        // Adds the cpu implementation to the schedules output
        cout << "[stage2][gpu] Realizing..." << endl;
        hIn.set_host_dirty();
        blury.realize(hOut);
        hOut.copy_to_host();
        cout << "[stage2][gpu] Done..." << endl;
    }
} stage2_gpu;

// Explicit registering required since startupSystem is called before genStage
// thus, only the manager node would have the stages registered
bool r1 = RTF::AutoStage::registerStage(&stage1_cpu);
bool r2 = RTF::AutoStage::registerStage(&stage1_gpu);
bool r3 = RTF::AutoStage::registerStage(&stage2_cpu);
bool r4 = RTF::AutoStage::registerStage(&stage2_gpu);

int main(int argc, char *argv[]) {

    // Manages inputs
    if (argc < 3) {
        cout << "usage: ./iwpp <I image> <J image> -h " 
            << "-c <number of cpu threads per node> " 
            << "-g <number of gpu threads per node> " << endl;
        cout << "\t-h is required for pipelines without implementations "
            << "of stages for every target." << endl;
        return 0;
    }
    // cv::Mat* cvI = new cv::Mat(cv::imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE));
    // cv::Mat* cvJ = new cv::Mat(cv::imread(argv[2], CV_LOAD_IMAGE_GRAYSCALE));

    // =========== trying v0.4 === Using autoTiling for breaking the image 
    // =========================== into RT tiles before sending for execution.
    SysEnv sysEnv;
    sysEnv.startupSystem(argc, argv, "libautostage.so");
    
    // Creates the inputs using RT's autoTiler
    int border = 0;
    int bgThr = 100;
    int erode = 4;
    int dilate = 10;
    int nTiles = 10;
    BGMasker* bgm = new ThresholdBGMasker(bgThr, dilate, erode);
    TiledRTCollection* tCollImgI = new IrregTiledRTCollection("inI", 
        "inI", argv[1], border, bgm, 
        NO_PRE_TILER, HBAL_TRIE_QUAD_TREE_ALG, nTiles);
    TiledRTCollection* tCollImgJ = new IrregTiledRTCollection("inJ", 
        "inJ", "./", border, bgm);

    tCollImgI->addImage(argv[1]);
    tCollImgJ->addImage(argv[2]);

    tCollImgI->tileImages();
    tCollImgJ->tileImages(tCollImgI->getTiles());

    // Create an instance of the two stages for each image tile pair
    // and also send them for execution
    std::vector<cv::Rect_<int>> tiles;
    std::list<cv::Rect_<int64_t>> l = tCollImgI->getTiles()[0];
    for (cv::Rect_<int64_t> tile : l) {
        // std::cout << tile.x << ":" << tile.width << "," 
        //     << tile.y << ":" << tile.height << std::endl;
        tiles.emplace_back(tile);
    }

    RegionTemplate* rtPropg = newRT("rtPropg");
    RegionTemplate* rtBlured = newRT("rtBlured");
    for (int i=0; i<tCollImgI->getNumRTs(); i++) {

        // Halide stage was created externally as stage1_hal

        RTF::AutoStage stage1({tCollImgI->getRT(i).second, 
            tCollImgJ->getRT(i).second, rtPropg}, {}, 
            // {tiles[i].height, tiles[i].width}, {&stage1_cpu, &stage1_gpu}, i);
            {tiles[i].height, tiles[i].width}, {&stage1_cpu}, i);

        RTF::AutoStage stage2({rtPropg, rtBlured}, {}, {tiles[i].height, 
            tiles[i].width}, {&stage2_gpu}, i);
        stage2.after(&stage1);

        stage2.genStage(sysEnv);
    }

    cout << "started" << endl;
    sysEnv.startupExecution();
    sysEnv.finalizeSystem();

    // // =========== Working v0.3 === Using RTF for execution pipelined stages
    // // Don't work with GPU threads, unless all tasks have gpu implementations.
    // // Creates the inputs
    // RegionTemplate* rtI = newRT(argv[1], cvI);
    // RegionTemplate* rtJ = newRT(argv[2], cvJ);
    // RegionTemplate* rtPropg = newRT("rtPropg");
    // RegionTemplate* rtBlured = newRT("rtBlured");

    // // Halide stage was created externally as stage1_cpu and others

    // RTF::AutoStage stage1({rtI, rtJ, rtPropg}, {}, {cvI->rows, cvI->cols}, 
    //     {&stage1_cpu, &stage1_gpu});

    // RTF::AutoStage stage2({rtPropg, rtBlured}, {}, {cvI->rows, cvI->cols}, 
    //     {&stage2_cpu});
    // stage2.after(&stage1);
    // stage2.execute(argc, argv);

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
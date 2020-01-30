#include <iostream>
#include <stdio.h>

#include "cv.hpp"
#include "Halide.h"

#include "AutoStage.h"
#include "IwppRecon.h"
#include "RegionTemplate.h"
#include "ExecEngineConstants.h"
#include "Util.h"

#include "TiledRTCollection.h"
#include "RegTiledRTCollection.h"
#include "IrregTiledRTCollection.h"
#include "HybridDenseTiledRTCollection.h"
#include "costFuncs/BGMasker.h"

#include "CostFunction.h"
#include "costFuncs/ThresholdBGMasker.h"
#include "costFuncs/ThresholdBGCostFunction.h"
#include "costFuncs/OracleCostFunction.h"

using std::cout;
using std::endl;

#define PROFILING
#define PROFILING_STAGES

enum TilingAlgorithm_t {
    NO_TILING,
    CPU_DENSE,
    HYBRID_DENSE,
    HYBRID_PRETILER,
    HYBRID_RESSPLIT,
};

// Should use ExecEngineConstants::GPU ... 
typedef int Target_t;
inline Target_t tgt(TilingAlgorithm_t tilingAlg, Target_t target) {
    switch (tilingAlg) {
        case NO_TILING:
        case CPU_DENSE:
            return ExecEngineConstants::CPU;
        case HYBRID_DENSE:
        case HYBRID_PRETILER:
        case HYBRID_RESSPLIT:
            return target;
    }
}

int findArgPos(std::string s, int argc, char** argv) {
    for (int i=1; i<argc; i++)
        if (std::string(argv[i]).compare(s)==0)
            return i;
    return -1;
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
Halide::Buffer<T> mat2buf(cv::Mat* m, std::string name="unnamed") {
    T* data = (T*)m->data;
    if (m->channels() > 1) {
        // Halide works with planar memory layout by default thus we need 
        // to ensure that it wraps the interleaved representation of opencv
        // correctly. This way, we can use the standard x,y,c indexing.
        // Still needs to set the Func's stride if the output Buffer also has
        // 3 channels:
        // func.output_buffer().dim(0).set_stride(3);
        return Halide::Buffer<T>::make_interleaved(data, 
            m->cols, m->rows, m->channels(), name);
    } else {
        return Halide::Buffer<T>(data, m->cols, m->rows, name);
    }

}

// CPU sched still marginally better since data is copied to and from device
// on every propagation loop instance
// usage: Halide::Func f; 
//        f.define_extern*({all inputs and outputs}); 
//        f.realize(); // without output buffer
template <typename T>
int loopedIwppRecon(halide_buffer_t* bII, halide_buffer_t* bJJ,
    int sched, halide_buffer_t* bOut, int cvDataType);
extern "C" int loopedIwppRecon(halide_buffer_t* bII, halide_buffer_t* bJJ,
    int sched, int T, halide_buffer_t* bOut) {
    switch (T) {
        case halide_type_uint:
            return loopedIwppRecon<uint8_t>(bII, bJJ, sched, bOut, CV_8U);
        case halide_type_int:
            return loopedIwppRecon<int32_t>(bII, bJJ, sched, bOut, CV_32S);
    }
}

// #define ANTIRASTER

template <typename T>
int loopedIwppRecon(halide_buffer_t* bII, halide_buffer_t* bJJ,
    int sched, halide_buffer_t* bOut, int cvDataType) {

    // cout << "[loopedIwppRecon] init" << endl;

    #ifdef PROFILING_STAGES
    long st0 = Util::ClockGetTime();
    #endif

    Halide::Buffer<T> II(*bII, "II");
    Halide::Buffer<T> JJ(*bJJ, "JJ");
    Halide::Buffer<T> hOut(*bOut, "hOut");
    int32_t w = bII->dim[0].extent;
    int32_t h = bII->dim[1].extent;
    // cv::imwrite("hOut.png", cv::Mat(h, w, 
    //     cv::DataType<T>::type, hOut.get()->raw_buffer()->host));

    Halide::Func rasterx("rasterx"), rastery("rastery");
    Halide::Func trsp1("trsp1"), trsp2("trsp2");
    Halide::Func arasterx("arasterx"), arastery("arastery");
    Halide::RDom se(-1,3,-1,3);
    Halide::Var x("x"), y("y");

    // Clamping inputs
    Halide::Func I = Halide::BoundaryConditions::repeat_edge(II);
    Halide::Func J = Halide::BoundaryConditions::repeat_edge(JJ);


    // Clamping rasterx for rastery input
    Halide::Expr xc = clamp(x, 0, w-1);
    Halide::Expr yc = clamp(y, 0, h-1);
    Halide::Func rasterxc("rasterxc");

    // Functions for vertical and horizontal propagation
    rasterx(x,y) = min(I(x,y), maximum(J(x+se.x,y)));
    rasterxc(x,y) = rasterx(xc,yc);
    rastery(x,y) = min(I(x,y), maximum(rasterxc(x,y+se.y)));

    #ifdef ANTIRASTER
    trsp1(x,y) = rastery(w-x-1,h-y-1);

    arasterx(x,y) = min(I(w-x-1,h-y-1), maximum(trsp1(clamp(x+se.x,0,w-1),y)));
    arastery(x,y) = min(I(w-x-1,h-y-1), maximum(arasterx(x,clamp(y+se.y,0,h-1))));

    trsp2(x,y) = arastery(w-x-1,h-y-1);
    #endif
    
    // Schedule
    rasterx.compute_root();
    Halide::Target target = Halide::get_host_target();
    if (sched == ExecEngineConstants::CPU) {
        Halide::Var xi, xo, yi, yo;
        rasterx.split(y, yo, yi, 16);
        rasterx.vectorize(yi).parallel(yo);
        rastery.reorder(y,x).serial(y);
        rastery.split(x, xo, xi, 16);
        rastery.vectorize(xi).parallel(xo);
    } else if (sched == ExecEngineConstants::GPU) {
        Halide::Var bx, by, tx, ty;
        rasterx.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        rastery.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        target.set_feature(Halide::Target::CUDA);
        II.set_host_dirty();
        JJ.set_host_dirty();
    }


    #ifdef ANTIRASTER
    Halide::Var xi, xo, yi, yo;
    arasterx.split(y, yo, yi, 16);
    arasterx.vectorize(yi).parallel(yo);
    arastery.reorder(y,x).serial(y);
    arastery.split(x, xo, xi, 16);
    arastery.vectorize(xi).parallel(xo);

    rastery.compute_root();
    arasterx.compute_root();    
    arastery.compute_root();
    #endif

    // target.set_feature(Halide::Target::Debug);
    #ifdef ANTIRASTER
    trsp2.compile_jit(target);
    #else
    rastery.compile_jit(target);
    #endif
    // rastery.compile_to_lowered_stmt("raster.html", {}, Halide::HTML);

    #ifdef PROFILING_STAGES
    long st1 = Util::ClockGetTime();
    cout << "[PROFILING][IWPP_COMP] " << (st1-st0) << endl;
    #endif

    unsigned long oldSum = 0;
    unsigned long newSum = 0;
    unsigned long it = 0;

    // Create a cv::Mat wrapper for the halide pipeline output buffer
    unsigned long iin = 1;

    do {

        #ifdef PROFILING_STAGES2
        long st2 = Util::ClockGetTime();
        #endif

        it++;
        oldSum = newSum;
        #ifdef ANTIRASTER
        trsp2.realize(JJ);
        #else
        rastery.realize(JJ);
        #endif
        // Copy from GPU to host is done every realization which is
        // inefficient. However this is just done as a proof of concept for
        // having a CPU and a GPU sched (more work necessary for running the 
        // sum on GPU). 
        if (sched == ExecEngineConstants::GPU) {
            JJ.copy_to_host();
        }

        #ifdef PROFILING_STAGES2
        long st3 = Util::ClockGetTime();
        #endif

        newSum = cv::sum(cv::Mat(h, w, cvDataType, JJ.get()->raw_buffer()->host))[0];
        // if (it%10 == 0 && iin > 0) {
        //     cv::Mat cvJ(h, w, cvDataType, JJ.get()->raw_buffer()->host);
        //     cv::imwrite("out.png", cvJ);
        //     cout << "out" << endl;
        //     std::cin >> iin;
        // }

        #ifdef PROFILING_STAGES2
        long st4 = Util::ClockGetTime();
        cout << "[PROFILING][IWPP_REALZ] " << (st3-st2) << endl;
        cout << "[PROFILING][IWPP_SUM] " << (st4-st3) << endl;
        #endif

    } while(newSum != oldSum);

    #ifdef PROFILING_STAGES
    long st5 = Util::ClockGetTime();
    std::cout << "[PROFILING][IWPP] iterations: " << it << std::endl;
    #endif

    hOut.copy_from(JJ); // bad
    // cv::imwrite("loopedIwppRecon.png", 
    //    cv::Mat(h, w, cvDataType, hOut.get()->raw_buffer()->host));

    #ifdef PROFILING_STAGES2
    long st6 = Util::ClockGetTime();
    std::cout << "[PROFILING][IWPP_CPY] " << (st6-st5) << std::endl;
    #endif

    return 0;
}

// int id=0;
// Needs to be static for referencing across mpi processes/nodes
static struct : RTF::HalGen {
    std::string getName() {return "get_background";}
    // int getTarget() {return ExecEngineConstants::CPU;}
    bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
                 std::vector<ArgumentBase*>& params) {

        #ifdef PROFILING_STAGES
        long st0 = Util::ClockGetTime();
        #endif

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
        // std::string name = "input" + std::to_string(id++) + ".tiff";
        // cv::imwrite(name, im_ios[0]);
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1], "hOut");

        uint8_t blue = ((ArgumentInt*)params[0])->getArgValue();
        uint8_t green = ((ArgumentInt*)params[1])->getArgValue();
        uint8_t red = ((ArgumentInt*)params[2])->getArgValue();

        int tileId = ((ArgumentInt*)params[3])->getArgValue();

        // Define halide stage
        Halide::Var x, y, c;
        Halide::Func get_bg;

        get_bg(x,y) = 255 * Halide::cast<uint8_t>(
            (hIn(x,y,0) > blue) & (hIn(x,y,1) > green) & (hIn(x,y,2) > red));

        #ifdef PROFILING_STAGES
        long st1 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_PREP][get_background] " << (st1-st0) << endl;
        #endif
        
        cout << "[get_background][cpu] Compiling..." << endl;
        get_bg.compile_jit();

        #ifdef PROFILING_STAGES
        long st2 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_COMP][get_background] " << (st2-st1) << endl;
        #endif

        cout << "[get_background][cpu] Realizing..." << endl;
        get_bg.realize(hOut);
        cout << "[get_background][cpu] Done..." << endl;

        #ifdef PROFILING_STAGES
        long st3 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_EXEC][get_background] " << (st3-st2) << endl;
        #endif

        int bgArea = cv::countNonZero(im_ios[1]);
        float ratio = (float)bgArea / (float)(im_ios[1].size().area());
        // check if there is too much background
        std::cout << "[get_background] ratio: " << ratio << std::endl;
        if (ratio >= 0.9) {
            std::cout << "[get_background] aborted!" << std::endl;
            return true; // abort if more than 90% background
        } else
            return false;
    }
} get_background;
bool r1 = RTF::AutoStage::registerStage(&get_background);

static struct : RTF::HalGen {
    std::string getName() {return "get_rbc";}
    // int getTarget() {return ExecEngineConstants::CPU;}
    bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
                 std::vector<ArgumentBase*>& params) {

        #ifdef PROFILING_STAGES
        long st0 = Util::ClockGetTime();
        #endif

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[2], "hOut");

        // Get params
        float T1 = ((ArgumentFloat*)params[0])->getArgValue();
        float T2 = ((ArgumentFloat*)params[1])->getArgValue();

        int tileId = ((ArgumentInt*)params[2])->getArgValue();        

        // Basic assertions
        assert(im_ios[0].channels() == 3);

        // Analog to cv::Mat::convertTo
        Halide::Var pix, eps;
        Halide::Func convertTo("convertTo");
        // Minor workaround with select since eps must be an int
        float alpha = 1.0f;
        float beta = FLT_EPSILON;
        convertTo(pix,eps) = alpha*Halide::cast<float>(pix)
                           + select(eps==1, beta, 0.0f);

        // Define halide stage
        Halide::Var x, y;
        Halide::Func bd("bd"), gd("gd"), rd("rd");
        Halide::Func imR2G("imR2G"), imR2B("imR2B");
        Halide::Func bw1("bw1"), bw2("bw2");
        Halide::Func marker("marker");
        Halide::Func rbc("rbc");
        Halide::Func rbc2("rbc2");

        cv::Mat cvbw1(hIn.height(), hIn.width(), CV_8U);
        cv::Mat cvbw2(hIn.height(), hIn.width(), CV_8U);
        cv::Mat cvmarker(hIn.height(), hIn.width(), CV_8U);
        Halide::Buffer<uint8_t> hbw1 = mat2buf<uint8_t>(&cvbw1, "hbw1");
        Halide::Buffer<uint8_t> hbw2 = mat2buf<uint8_t>(&cvbw2, "hbw1");
        Halide::Buffer<uint8_t> hmarker = mat2buf<uint8_t>(&cvmarker, "hmarker");

        bd(x,y) = convertTo(hIn(x,y,0), 1);
        gd(x,y) = convertTo(hIn(x,y,1), 1);
        rd(x,y) = convertTo(hIn(x,y,2), 0);

        imR2G(x,y) = rd(x,y)/gd(x,y);
        imR2B(x,y) = (rd(x,y)/bd(x,y)) > 1.0f;

        bw1(x,y) = Halide::cast<uint8_t>(imR2G(x,y) > T1);
        bw2(x,y) = Halide::cast<uint8_t>(imR2G(x,y) > T2)*255;

        #ifdef PROFILING_STAGES
        long st1 = Util::ClockGetTime();
        #endif
        
        cout << "[get_rbc][cpu] Compiling..." << endl;
        bw1.compile_jit();

        #ifdef PROFILING_STAGES
        long st2 = Util::ClockGetTime();
        #endif

        // Realizes bw1 to identify if any pixels were found
        cout << "[get_rbc][cpu] Realizing pre-rbc..." << endl;
        bw1.realize(hbw1);
        
        #ifdef PROFILING_STAGES
        long st3 = Util::ClockGetTime();
        #endif

        int nonz = cv::countNonZero(cvbw1);
        cout << "[get_rbc][cpu] Done pre-rbc..." << endl;

        // Mat marker = Mat::zeros(bw1.size(), bw1.type());
        // bw2.copyTo(marker, bw1);
        marker(x,y) = Halide::select(hbw1(x,y)>0, bw2(x,y), 0);

        // Propagation part removed since it don't currently affect the output
        // // Only goes further for the propagation if there is any non-zero pixel
        // if (nonz > 0) {
        //     cout << "[get_rbc][cpu] non-zero: " << nonz << endl;

        //     // These two must be scheduled this way since they are inputs
        //     // for an extern halide function
        //     marker.compute_root();
        //     bw2.compute_root();

        //     // marker = imreconstructBinary<T>(marker, bw2, connectivity);
        //     rbc.define_extern("loopedIwppRecon", {hmarker, hbw2, 
        //         Halide::UInt(8), ExecEngineConstants::CPU}, 
        //         Halide::UInt(8), 2);

        // } else {
        //     cout << "[get_rbc][cpu] No propagation" << endl;
        //     rbc(x,y) = 0;
        // }
        // rbc2(x,y) = rbc(x,y) & marker(x,y) & imR2B(x,y);
        
        rbc2(x,y) = marker(x,y) & imR2B(x,y);
        // rbc.compute_root();

        #ifdef PROFILING_STAGES
        long st4 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_PREP][get_rbc] " << (st4-st3+st1-st0) << endl;
        #endif
        
        cout << "[get_rbc][cpu] Compiling..." << endl;
        rbc2.compile_jit();

        #ifdef PROFILING_STAGES
        long st5 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_COMP][get_rbc] " << (st5-st4+st2-st1) << endl;
        #endif

        cout << "[get_rbc][cpu] Realizing propagation..." << endl;
        rbc2.realize(hOut);
        cout << "[get_rbc][cpu] Done" << endl;
        
        #ifdef PROFILING_STAGES
        long st6 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_EXEC][get_rbc] " << (st6-st5+st3-st2) << endl;
        #endif

        return false;
    }
} get_rbc;
bool r2 = RTF::AutoStage::registerStage(&get_rbc);

static struct : RTF::HalGen {
    std::string getName() {return "invert";}
    // int getTarget() {return ExecEngineConstants::CPU;}
    bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
                 std::vector<ArgumentBase*>& params) {

        if (im_ios.size() != 3) {
            std::cout << "[invert] missing RTs: input, abortion flag, output" << std::endl;
            exit(-1);
        }

        #ifdef PROFILING_STAGES
        long st0 = Util::ClockGetTime();
        #endif

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[2], "hOut");

        // Get params
        // channel to be inverted (if -1, then input is grayscale)
        int channel = ((ArgumentInt*)params[0])->getArgValue();
        int tileId = ((ArgumentInt*)params[1])->getArgValue();

        if (channel == -1)
            assert(im_ios[0].channels() == 1);
        else
            assert(im_ios[0].channels() == 3);

        // Define halide stage
        Halide::Var x, y;
        Halide::Func invert;

        if (channel == -1)
            invert(x,y) = std::numeric_limits<uint8_t>::max()-hIn(x,y);
        else
            invert(x,y) = std::numeric_limits<uint8_t>::max()-hIn(x,y,channel);

        #ifdef PROFILING_STAGES
        long st1 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_PREP][invert] " << (st1-st0) << endl;
        #endif
        
        cout << "[invert][cpu] Compiling..." << endl;
        invert.compile_jit();

        #ifdef PROFILING_STAGES
        long st2 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_COMP][invert] " << (st2-st1) << endl;
        #endif
        
        cout << "[invert][cpu] Realizing..." << endl;
        invert.realize(hOut);
        cout << "[invert][cpu] Done..." << endl;

        #ifdef PROFILING_STAGES
        long st3 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_EXEC][invert] " << (st3-st2) << endl;
        #endif

        return false;
    }
} invert;
bool r3 = RTF::AutoStage::registerStage(&invert);

// Only implemented for grayscale images
// Only implemented for uint8_t images
// Structuring element must have odd width and height
static struct : RTF::HalGen {
    std::string getName() {return "erode";}
    // int getTarget() {return ExecEngineConstants::CPU;}
    bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
                 std::vector<ArgumentBase*>& params) {

        #ifdef PROFILING_STAGES
        long st0 = Util::ClockGetTime();
        #endif

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1], "hOut");

        // Get params
        int disk19raw_width = ((ArgumentInt*)params[0])->getArgValue();
        int* disk19raw = ((ArgumentIntArray*)params[1])->getArgValue();
        cv::Mat cvSE(disk19raw_width, disk19raw_width, CV_8U);
        for (int i=0; i<cvSE.cols; i++) {
            for (int j=0; j<cvSE.rows; j++) {
                cvSE.at<uint8_t>(i,j) = disk19raw[i+j*disk19raw_width];
            }
        }
        Halide::Buffer<uint8_t> hSE = mat2buf<uint8_t>(&cvSE, "hSE");
        int tileId = ((ArgumentInt*)params[2])->getArgValue();
        
        // basic assertions for that ensures that this will work
        assert(hSE.width()%2 != 0);
        assert(hSE.height()%2 != 0);
        assert(im_ios[0].channels() == 1);

        int seWidth = (hSE.width()-1)/2;
        int seHeight = (hSE.height()-1)/2;

        // Define halide stage
        Halide::Var x, y, xi, yi;
        Halide::Func mask;
        Halide::Func erode;

        // Definition of a sub-area on which 
        // mask(x,y) = hSE(x,y)==1 ? hIn(x,y) : 255
        mask(x,y,xi,yi) = hSE(xi,yi)*hIn(x+xi-seWidth,y+yi-seHeight) 
            + (1-hSE(xi,yi))*255;

        Halide::Expr xc = clamp(x, seWidth, hIn.width()-seWidth);
        Halide::Expr yc = clamp(y, seHeight, hIn.height()-seHeight);
        Halide::RDom se(0, hSE.width()-1, 0, hSE.height()-1);
        erode(x,y) = minimum(mask(xc,yc,se.x,se.y));

        // Schedules
        Halide::Var pix;
        erode.fuse(x,y,pix).parallel(pix);

        #ifdef PROFILING_STAGES
        long st1 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_PREP][erode] " << (st1-st0) << endl;
        #endif
        
        cout << "[erode][cpu] Compiling..." << endl;
        erode.compile_jit();

        #ifdef PROFILING_STAGES
        long st2 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_COMP][erode] " << (st2-st1) << endl;
        #endif

        cout << "[erode][cpu] Realizing..." << endl;
        erode.realize(hOut);
        cout << "[erode][cpu] Done..." << endl;

        #ifdef PROFILING_STAGES
        long st3 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_EXEC][erode] " << (st3-st2) << endl;
        #endif

        return false;
    }
} erode;
bool r4 = RTF::AutoStage::registerStage(&erode);

static struct : RTF::HalGen {
    std::string getName() {return "dilate";}
    // int getTarget() {return ExecEngineConstants::CPU;}
    bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
                 std::vector<ArgumentBase*>& params) {

        #ifdef PROFILING_STAGES
        long st0 = Util::ClockGetTime();
        #endif

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1], "hOut");

        // Get params
        int disk19raw_width = ((ArgumentInt*)params[0])->getArgValue();
        int* disk19raw = ((ArgumentIntArray*)params[1])->getArgValue();
        cv::Mat cvSE(disk19raw_width, disk19raw_width, CV_8U);
        for (int i=0; i<cvSE.cols; i++) {
            for (int j=0; j<cvSE.rows; j++) {
                cvSE.at<uint8_t>(i,j) = disk19raw[i+j*disk19raw_width];
            }
        }
        Halide::Buffer<uint8_t> hSE = mat2buf<uint8_t>(&cvSE, "hSE");
        int tileId = ((ArgumentInt*)params[2])->getArgValue();

        // sizes must be odd
        assert(hSE.width()%2 == 1);
        assert(hSE.height()%2 == 1);

        int seWidth = (hSE.width()-1)/2;
        int seHeight = (hSE.height()-1)/2;

        // Define halide stage
        Halide::Var x, y, xi, yi;
        Halide::Func mask;
        Halide::Func dilate;

        // Definition of a sub-area on which 
        // mask(x,y) = hSE(x,y)==1 ? hIn(x,y) : 0
        mask(x,y,xi,yi) = hSE(xi,yi)*hIn(x+xi-seWidth,y+yi-seHeight);

        Halide::Expr xc = clamp(x, seWidth, hIn.width()-seWidth);
        Halide::Expr yc = clamp(y, seHeight, hIn.height()-seHeight);
        Halide::RDom se(0, hSE.width()-1, 0, hSE.height()-1);
        dilate(x,y) = maximum(mask(xc,yc,se.x,se.y));

        // Schedules
        Halide::Var pix;
        dilate.fuse(x,y,pix).parallel(pix);

        #ifdef PROFILING_STAGES
        long st1 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_PREP][dilate] " << (st1-st0) << endl;
        #endif
        
        cout << "[dilate][cpu] Compiling..." << endl;
        dilate.compile_jit();

        #ifdef PROFILING_STAGES
        long st2 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_COMP][dilate] " << (st2-st1) << endl;
        #endif

        cout << "[dilate][cpu] Realizing..." << endl;
        dilate.realize(hOut);
        cout << "[dilate][cpu] Done..." << endl;

        #ifdef PROFILING_STAGES
        long st3 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_EXEC][dilate] " << (st3-st2) << endl;
        #endif

        return false;
    }
} dilate;
bool r5 = RTF::AutoStage::registerStage(&dilate);

static struct : RTF::HalGen {
    std::string getName() {return "pre_fill_holes";}
    // int getTarget() {return ExecEngineConstants::CPU;}
    bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
                 std::vector<ArgumentBase*>& params) {

        #ifdef PROFILING_STAGES
        long st0 = Util::ClockGetTime();
        #endif

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hRecon = mat2buf<uint8_t>(&im_ios[0], "cvRecon");
        Halide::Buffer<uint8_t> hRC = mat2buf<uint8_t>(&im_ios[1], "hRC");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[2], "hOut");

        // Get params
        int G1 = ((ArgumentInt*)params[0])->getArgValue();
        int tileId = ((ArgumentInt*)params[1])->getArgValue();

        // sizes must be odd
        assert(im_ios[0].channels() == 1);
        assert(im_ios[1].channels() == 1);

        // Define halide stage
        Halide::Var x, y;
        Halide::Func preFill;

        preFill(x,y) = 255*Halide::cast<uint8_t>((hRC(x,y) - hRecon(x,y)) > G1);

        // Set the borders as -inf (i.e., 0);
        preFill(x,0) = Halide::cast<uint8_t>(0);
        preFill(x,hRecon.height()-1) = Halide::cast<uint8_t>(0);
        preFill(0,y) = Halide::cast<uint8_t>(0);
        preFill(hRecon.width()-1,y) = Halide::cast<uint8_t>(0);

        // Schedules
        preFill.compute_root();
        preFill.parallel(x);

        #ifdef PROFILING_STAGES
        long st1 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_PREP][pre_fill_holes] " << (st1-st0) << endl;
        #endif
        
        cout << "[pre_fill_holes][cpu] Compiling..." << endl;
        preFill.compile_jit();

        #ifdef PROFILING_STAGES
        long st2 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_COMP][pre_fill_holes] " << (st2-st1) << endl;
        #endif

        cout << "[pre_fill_holes][cpu] Realizing..." << endl;
        preFill.realize(hOut);
        cout << "[pre_fill_holes][cpu] Done..." << endl;

        #ifdef PROFILING_STAGES
        long st3 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_EXEC][pre_fill_holes] " << (st3-st2) << endl;
        #endif

        return false;
    }
} pre_fill_holes;
bool r6 = RTF::AutoStage::registerStage(&pre_fill_holes);

// Needs to be static for referencing across mpi processes/nodes
static struct : RTF::HalGen {
    std::string getName() {return "imreconstruct";}
    // int getTarget() {return ExecEngineConstants::CPU;}
    bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
                 std::vector<ArgumentBase*>& params) {

        #ifdef PROFILING_STAGES
        long st0 = Util::ClockGetTime();
        #endif

        // Get params
        int tileId = ((ArgumentInt*)params[2])->getArgValue();
        // channel to be inverted (if -1, then input is grayscale)
        int emptyAny = ((ArgumentInt*)params[0])->getArgValue();
        cv::Mat *cvI, *cvJ, *cvOut;
        if (emptyAny < 0) {
            // I is empty
            cout << "[imreconstruct] empty I" << endl;
            int fillVal = ((ArgumentInt*)params[1])->getArgValue();
            cvI = new cv::Mat(im_ios[0].size(), im_ios[0].type(), 
                cv::Scalar(fillVal));
            cvJ = &im_ios[0];
            cvOut = &im_ios[1];
        } else if (emptyAny > 0) {
            // J is empty
            cout << "[imreconstruct] empty J" << endl;
            int fillVal = ((ArgumentInt*)params[1])->getArgValue();
            cvJ = new cv::Mat(im_ios[0].size(), im_ios[0].type(), 
                cv::Scalar(fillVal));
            cvI = &im_ios[0];
            cvOut = &im_ios[1];
        } else {
            // none is empty
            cvI = &im_ios[0];
            cvJ = &im_ios[1];
            cvOut = &im_ios[2];
        }

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hI = mat2buf<uint8_t>(cvI, "hI");
        Halide::Buffer<uint8_t> hJ = mat2buf<uint8_t>(cvJ, "hJ");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(cvOut, "hOut");

        // Define halide stage
        Halide::Func halCpu("halCpu");
        halCpu.define_extern("loopedIwppRecon", {hI, hJ, target, 
            Halide::UInt(8).code(), hOut}, Halide::UInt(8), 2);

        #ifdef PROFILING_STAGES
        long st1 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_PREP][imreconstruct] " << (st1-st0) << endl;
        #endif
        
        cout << "[imreconstruct][cpu] Compiling..." << endl;
        halCpu.compile_jit();

        #ifdef PROFILING_STAGES
        long st2 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_COMP][imreconstruct] " << (st2-st1) << endl;
        #endif

        // Adds the cpu implementation to the schedules output
        cout << "[imreconstruct][cpu] Realizing..." << endl;
        halCpu.realize(hOut);
        cout << "[imreconstruct][cpu] Done..." << endl;

        #ifdef PROFILING_STAGES
        long st3 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_EXEC][imreconstruct] " << (st3-st2) << endl;
        #endif

        return false;
    }
} imreconstruct;
bool r7 = RTF::AutoStage::registerStage(&imreconstruct);

static struct : RTF::HalGen {
    std::string getName() {return "pre_fill_holes2";}
    // int getTarget() {return ExecEngineConstants::CPU;}
    bool realize(std::vector<cv::Mat>& im_ios, Target_t target, 
                 std::vector<ArgumentBase*>& params) {

        #ifdef PROFILING_STAGES
        long st0 = Util::ClockGetTime();
        #endif

        // Wraps the input and output cv::mat's with halide buffers
        // hIn = pre_fill = image
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1], "hOut");

        // get params
        int tileId = ((ArgumentInt*)params[0])->getArgValue();

        // input image must be grayscale
        assert(im_ios[0].channels() == 1);

        // Creates the marker image
        int32_t mn = std::numeric_limits<int32_t>::min();
        int32_t mx = std::numeric_limits<int32_t>::max();
        cv::Mat cvMk(im_ios[0].size(), CV_32S);
        cv::Mat mk2(im_ios[0].rows-2, im_ios[0].cols-2, CV_32S, cv::Scalar(mn));
        // Them make the border - OpenCV does not replicate the values 
        // when one cv::Mat is a region of another.
        cv::copyMakeBorder(mk2, cvMk, 1, 1, 1, 1, cv::BORDER_CONSTANT, mx);
        Halide::Buffer<int32_t> hMarker = mat2buf<int32_t>(&cvMk, "marker");

        // Creates the propagation output int32 image
        cv::Mat cvRecon(im_ios[0].size(), CV_32S, cv::Scalar(0));
        Halide::Buffer<int32_t> hRecon = mat2buf<int32_t>(&cvRecon, "recon");

        // Define halide stage
        Halide::Var x, y;
        Halide::Func hIn32("hIn32");
        Halide::Func recon("recon");
        Halide::Func output("output");

        cout << "hRecon: " << hRecon.width() << "x" << hRecon.height() << endl;
        cout << "hMarker: " << hMarker.width() << "x" << hMarker.height() << endl;

        recon.define_extern("loopedIwppRecon", {hIn, hMarker, 
            target, Halide::Int(32).code(), hRecon}, 
            Halide::Int(32), 2);

        // Schedules
        hIn32.compute_root();
        recon.compute_root();

        #ifdef PROFILING_STAGES
        long st1 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_PREP][pre_fill_holes2] " << (st1-st0) << endl;
        #endif
        
        cout << "[pre_fill_holes2][cpu] Compiling..." << endl;
        recon.compile_jit();

        #ifdef PROFILING_STAGES
        long st2 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_COMP][pre_fill_holes2] " << (st2-st1) << endl;
        #endif

        cout << "[pre_fill_holes2][cpu] Realizing..." << endl;
        recon.realize(hRecon);
        cout << "[pre_fill_holes2][cpu] Done..." << endl;
        
        #ifdef PROFILING_STAGES
        long st3 = Util::ClockGetTime();
        cout << "[PROFILING][" << tileId << "][STAGE_HAL_EXEC][pre_fill_holes2] " << (st3-st2) << endl;
        #endif

        return false;
    }
} pre_fill_holes2;
bool r8 = RTF::AutoStage::registerStage(&pre_fill_holes2);

int main(int argc, char *argv[]) {
    // Manages inputs
    if (argc < 2) {
        cout << "Usage: ./iwpp <I image> [ARGS]" << endl;
        cout << "\t-c <number of cpu threads per node "
             << "(default=1)>" << endl;
        cout << "\t-g <number of gpu threads per node "
             << "(default=0)>" << endl;
        cout << "\t-t <Number of tiles per resource thread "
             << " to be generated (default=1)>" << endl;
        cout << "\t-b <tiling border (default=0)>" << endl;
        cout << "\t-p <bgThr>/<erode>/<dilate> (default=100/4/10)" << endl;
        cout << "\t-to (tiling only: generate tile images "
             << "without executing)" << endl;

        cout << "\t-a <tiling algorithm>" << endl;
        cout << "\t\tValues (default=0):" << endl;
        cout << "\t\t0: No tiling (supports non-svs images)" << endl;
        cout << "\t\t1: CPU-only dense tiling" << endl;
        cout << "\t\t2: Hybrid dense tiling" << endl;
        cout << "\t\t3: Pre-tiler hybrid tiling" << endl;
        cout << "\t\t4: Res-split hybrid tiling" << endl;

        cout << "\t-d <dense tiling algorithm>" << endl;
        cout << "\t\tValues (default=0):" << endl;
        cout << "\t\t0: FIXED_GRID_TILING" << endl;
        cout << "\t\t1: LIST_ALG_HALF" << endl;
        cout << "\t\t2: LIST_ALG_EXPECT" << endl;
        cout << "\t\t3: KD_TREE_ALG_AREA" << endl;
        cout << "\t\t4: KD_TREE_ALG_COST" << endl;
        cout << "\t\t5: HBAL_TRIE_QUAD_TREE_ALG" << endl;
        cout << "\t\t6: CBAL_TRIE_QUAD_TREE_ALG" << endl;
        cout << "\t\t7: CBAL_POINT_QUAD_TREE_ALG" << endl;
        
        exit(0);
    }

    // Input images
    std::string Ipath = std::string(argv[1]);

    // Number of cpu threads
    int cpuThreads = 1;
    if (findArgPos("-c", argc, argv) != -1) {
        cpuThreads = atoi(argv[findArgPos("-c", argc, argv)+1]);
    }

    // Number of gpu threads
    int gpuThreads = 1;
    if (findArgPos("-g", argc, argv) != -1) {
        cpuThreads = atoi(argv[findArgPos("-g", argc, argv)+1]);
    }

    // Number of expected dense tiles for irregular tiling
    int nTilesPerThread = 1;
    if (findArgPos("-t", argc, argv) != -1) {
        nTilesPerThread = atoi(argv[findArgPos("-t", argc, argv)+1]);
    }

    // Pre-tiler parameters
    int bgThr = 100;
    int erode_param = 4;
    int dilate_param = 10;
    if (findArgPos("-p", argc, argv) != -1) {
        std::string params = argv[findArgPos("-p", argc, argv)+1];
        std::size_t l = params.find_last_of("/");
        dilate_param = atoi(params.substr(l+1).c_str());
        params = params.substr(0, l);
        l = params.find_last_of("/");
        erode_param = atoi(params.substr(l+1).c_str());
        bgThr = atoi(params.substr(0, l).c_str());
    }

    // Full tiling algorithm
    TilingAlgorithm_t tilingAlg = NO_TILING;
    if (findArgPos("-a", argc, argv) != -1) {
        tilingAlg = static_cast<TilingAlgorithm_t>(
            atoi(argv[findArgPos("-a", argc, argv)+1]));
    }

    // Dense tiling algorithm
    TilerAlg_t denseTilingAlg = FIXED_GRID_TILING;
    if (findArgPos("-d", argc, argv) != -1) {
        denseTilingAlg = static_cast<TilerAlg_t>(
            atoi(argv[findArgPos("-d", argc, argv)+1]));
    }

    int border = 0;
    if (findArgPos("-b", argc, argv) != -1) {
        border = atoi(argv[findArgPos("-b", argc, argv)+1]);
    }

    // Tiling only
    bool tilingOnly = false;
    if (findArgPos("-to", argc, argv) != -1) {
        tilingOnly = true;
    }

#ifdef PROFILING
    long fullExecT1 = Util::ClockGetTime();
#endif

    SysEnv sysEnv;
    sysEnv.startupSystem(argc, argv, "libautostage.so");

    // Input parameters
    unsigned char blue = 200;
    unsigned char green = 200;
    unsigned char red = 200;
    double T1 = 1.0;
    double T2 = 2.0;
    unsigned char G1 = 50;
    unsigned char G2 = 100;
    int minSize = 10;
    int maxSize = 100;
    int fillHolesConnectivity = 4;
    int reconConnectivity = 4;
    // 19x19
    int disk19raw_width = 19;
    int disk19raw_size = disk19raw_width*disk19raw_width;
    int disk19raw[disk19raw_size] = {
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
        0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
        0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
        0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
    
    // 3x3
    int se3raw_width = 3;
    int se3raw_size = se3raw_width*se3raw_width;
    int se3raw[se3raw_size] = {
        1, 1, 1,
        1, 1, 1, 
        1, 1, 1};
    
    // Creates the inputs using RT's autoTiler
#ifdef PROFILING
    long tilingT1 = Util::ClockGetTime();
#endif
    BGMasker* bgm = new ThresholdBGMasker(bgThr, dilate_param, erode_param);
    CostFunction* cfunc = new ThresholdBGCostFunction((ThresholdBGMasker*)bgm);
    
    TiledRTCollection* tCollImg;
    switch (tilingAlg) {
        case NO_TILING:
            tCollImg = new TiledRTCollection("input", "input", Ipath, cfunc);
            break;
        case CPU_DENSE: {
            int nTiles = nTilesPerThread*cpuThreads;
            if (denseTilingAlg == FIXED_GRID_TILING) {
                tCollImg = new RegTiledRTCollection("input", 
                    "input", Ipath, nTiles, border, cfunc);
            } else {
                tCollImg = new IrregTiledRTCollection("input", 
                    "input", Ipath, border, cfunc, bgm, 
                    denseTilingAlg, nTiles);
            }
            break;
        }
        case HYBRID_DENSE:
            tCollImg = new HybridDenseTiledRTCollection(
                "input", "input", Ipath, border, cfunc, bgm, denseTilingAlg, 
                nTilesPerThread*cpuThreads, nTilesPerThread*gpuThreads);
            break;
        case HYBRID_PRETILER:
            // HybridTiledRTCollection ht = new HybridTiledRTCollection(
            //     "input", "input", Ipath, {borderL1, borderL2}, 
            //     {cfuncR1, cfuncR2}, preTilerAlg, {tilerAlgR1, tilerAlgR2}, 
            //     {nTilesR1, nTilesR2});
            break;
        case HYBRID_RESSPLIT:
            break;
    }

    tCollImg->addImage(Ipath);
    tCollImg->tileImages(tilingOnly);

#ifdef PROFILING
    long tilingT2 = Util::ClockGetTime();
    cout << "[PROFILING][TILING_TIME] " << (tilingT2-tilingT1) << endl;
    cout << "[PROFILING][TILES] " << tCollImg->getNumRTs() << endl;
#endif

    if (tilingOnly) {
        sysEnv.startupExecution();
        sysEnv.finalizeSystem();
        return 0;
    }

    // Create an instance of the two stages for each image tile pair
    // and also send them for execution
    std::vector<cv::Rect_<int>> tiles;
    std::list<cv::Rect_<int64_t>> l = tCollImg->getTiles()[0];
    for (cv::Rect_<int64_t> tile : l) {
        // std::cout << tile.x << ":" << tile.width << "," 
        //     << tile.y << ":" << tile.height << std::endl;
        tiles.emplace_back(tile);
    }

    RegionTemplate* rtBackground = newRT("0rtBackground");
    RegionTemplate* rtRBC = newRT("1rtRBC");
    RegionTemplate* rtRC = newRT("2rtRC");
    RegionTemplate* rtEroded = newRT("3rtEroded");
    RegionTemplate* rtRcOpen = newRT("3rtRcOpen");
    RegionTemplate* rtRecon = newRT("4rtRecon");
    RegionTemplate* rtPreFill = newRT("5rtPreFill");
    RegionTemplate* rtInvRecon = newRT("5rtInvRecon");
    RegionTemplate* rtPreFill2 = newRT("5rtPreFill2");
    RegionTemplate* rtBw1 = newRT("6rtBw1");
    for (int i=0; i<tCollImg->getNumRTs(); i++) {

        // background = get_background(input)
        // bgArea = countNonZero(background) -> exit if ratio > 0.9
        RTF::AutoStage stage0({tCollImg->getRT(i).second, rtBackground}, 
            {new ArgumentInt(blue), new ArgumentInt(green), 
             new ArgumentInt(red), new ArgumentInt(i)}, 
            {tiles[i].height, tiles[i].width}, {&get_background}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage0.genStage(sysEnv);
        
        // rbc = get_rbc(input)
        // Added rtBackground for aborting signal
        RTF::AutoStage stage1({tCollImg->getRT(i).second, rtBackground, rtRBC}, 
            {new ArgumentFloat(T1), new ArgumentFloat(T2), new ArgumentInt(i)}, 
            {tiles[i].height, tiles[i].width}, {&get_rbc}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage1.after(&stage0);
        stage1.genStage(sysEnv);

        // rc = invert(input[2])
        // Added rtRBC for aborting signal
        RTF::AutoStage stage2({tCollImg->getRT(i).second, rtRBC, rtRC}, 
            {new ArgumentInt(0), new ArgumentInt(i)}, 
            {tiles[i].height, tiles[i].width}, {&invert}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage2.after(&stage1);
        stage2.genStage(sysEnv);
        
        // rc_open = morph_open(rc, disk19raw):
        // rc_open = dilate(erode(rc, disk19raw), disk19raw)
        RTF::AutoStage stage3({rtRC, rtEroded}, 
            {new ArgumentInt(disk19raw_width), 
             new ArgumentIntArray(disk19raw, disk19raw_size),
             new ArgumentInt(i)}, 
            {tiles[i].height, tiles[i].width}, {&erode}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage3.after(&stage2);
        stage3.genStage(sysEnv);
        RTF::AutoStage stage4({rtEroded, rtRcOpen}, 
            {new ArgumentInt(disk19raw_width), 
             new ArgumentIntArray(disk19raw, disk19raw_size),
             new ArgumentInt(i)}, 
            {tiles[i].height, tiles[i].width}, {&dilate}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage4.after(&stage3);
        stage4.genStage(sysEnv);

        RTF::AutoStage stage5({rtRC, rtRcOpen, rtRecon}, 
            {new ArgumentInt(0), new ArgumentInt(0), new ArgumentInt(i)}, 
            {tiles[i].height, tiles[i].width}, {&imreconstruct}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage5.after(&stage4);
        stage5.genStage(sysEnv);

        // pre_fill = (rc - imrec(rc_open, rc)) > G1
        RTF::AutoStage stage6({rtRecon, rtRC, rtPreFill}, 
            {new ArgumentInt(G1), new ArgumentInt(i)}, 
            {tiles[i].height, tiles[i].width}, {&pre_fill_holes}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage6.after(&stage5);
        stage6.genStage(sysEnv);

        /* FILL HOLES closing
         * Using closing algorithm: dilate then erode
         */
        RTF::AutoStage stage7({rtPreFill, rtPreFill2}, 
            {new ArgumentInt(se3raw_width), 
             new ArgumentIntArray(se3raw, se3raw_size),
             new ArgumentInt(i)}, 
            {tiles[i].height, tiles[i].width}, {&dilate}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage7.after(&stage6);
        stage7.genStage(sysEnv);
        RTF::AutoStage stage8({rtPreFill2, rtBw1}, 
            {new ArgumentInt(se3raw_width), 
             new ArgumentIntArray(se3raw, se3raw_size),
             new ArgumentInt(i)}, 
            {tiles[i].height, tiles[i].width}, {&erode}, 
            tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        stage8.after(&stage7);
        stage8.genStage(sysEnv);

        /* FILL HOLES WORKING
         * However it takes too log to execute using iwpp.
         */
        // // bw1 = fill_holes(pre_fill) = invert(imrec(invert(pre_fill)))
        // RTF::AutoStage stage7({rtPreFill, rtRBC, rtInvRecon}, 
        //     {new ArgumentInt(-1), new ArgumentInt(i)}, 
        //     {tiles[i].height, tiles[i].width}, {&invert}, 
        //     tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        // stage7.after(&stage6);
        // stage7.genStage(sysEnv);
        // RTF::AutoStage stage8({rtInvRecon, rtPreFill2}, {new ArgumentInt(i)}, 
        //     {tiles[i].height, tiles[i].width}, {&pre_fill_holes2}, 
        //     tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        // stage8.after(&stage7);
        // stage8.genStage(sysEnv);
        // RTF::AutoStage stage9({rtPreFill2, rtRBC, rtBw1},
        //     {new ArgumentInt(-1), new ArgumentInt(i)}, 
        //     {tiles[i].height, tiles[i].width}, {&invert}, 
        //     tgt(tilingAlg, tCollImg->getTileTarget(i)), i);
        // stage9.after(&stage8);
        // stage9.genStage(sysEnv);

        // // bw1_t,compcount2 = bwareaopen2(bw1) //-> exit if compcount2 == 0
        // -=- seg_norbc = bwselect(diffIm > G2, bw1_t) & (rbc == 0)
        // find_cand = seg_norbc

        // RTF::AutoStage stage2({rtPropg, rtBlured}, {}, {tiles[i].height, 
        //     tiles[i].width}, {&stage2_gpu}, i);
        // stage2.after(&stage1);
        // stage2.genStage(sysEnv);
    }

    // cout << "started" << endl;
    sysEnv.startupExecution();
    sysEnv.finalizeSystem();

#ifdef PROFILING
    long fullExecT2 = Util::ClockGetTime();
    cout << "[PROFILING][FULL_TIME] " << (fullExecT2-fullExecT1) << endl;
#endif

    return 0;
}
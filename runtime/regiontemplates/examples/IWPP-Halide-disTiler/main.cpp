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
    if (m->channels() > 1) {
        // Halide works with planar memory layout by default thus we need 
        // to ensure that it wraps the interleaved representation of opencv
        // correctly. This way, we can use the standard x,y,c indexing.
        // Still needs to set the Func's stride if the output Buffer also has
        // 3 channels:
        // func.output_buffer().dim(0).set_stride(3);
        return Halide::Buffer<uint8_t>::make_interleaved(
            m->data, m->cols, m->rows, m->channels(), name);
    } else
        return Halide::Buffer<uint8_t>(m->data, m->cols, m->rows, name);
}

// CPU sched still marginally better since data is copied to and from device
// on every propagation loop instance
extern "C" int loopedIwppRecon(halide_buffer_t* bII, halide_buffer_t* bJJ,
    int sched, halide_buffer_t* bOut) {

    cout << "[loopedIwppRecon] init" << endl;

    Halide::Buffer<uint8_t> II(*bII, "II");
    Halide::Buffer<uint8_t> inJJ(*bJJ, "inJJ");
    Halide::Buffer<uint8_t> JJ(*bOut, "JJ");

    JJ = inJJ.copy();

    Halide::Func rasterx("rasterx"), rastery("rastery"), arasterx, arastery;
    Halide::RDom se(-1,3,-1,3);
    Halide::Var x("x"), y("y");

    // Clamping inputs
    Halide::Func I = Halide::BoundaryConditions::repeat_edge(II);
    Halide::Func J = Halide::BoundaryConditions::repeat_edge(JJ);

    int32_t w = bII->dim[0].extent;
    int32_t h = bII->dim[1].extent;

    // Clamping rasterx for rastery input
    Halide::Expr xc = clamp(x, 0, w-1);
    Halide::Expr yc = clamp(y, 0, h-1);
    Halide::Func rasterxc("rasterxc");

    // Functions for vertical and horizontal propagation
    rasterx(x,y) = min(I(x,y), maximum(J(x+se.x,y)));
    // arasterx(w-x,y) = min(I(w-x,y), maximum(rasterx(w-x+se.x, y)));
    rasterxc(x,y) = rasterx(xc,yc);
    rastery(x,y) = min(I(x,y), maximum(rasterxc(x,y+se.y)));
    // arastery(x,h-y) = min(I(x,h-y), maximum(rastery(x, h-y+se.y)));

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
        rastery.compile_jit();
    } else if (sched == ExecEngineConstants::GPU) {
        Halide::Var bx, by, tx, ty;
        rasterx.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        rastery.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        target.set_feature(Halide::Target::CUDA);
        II.set_host_dirty();
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
        // Copy from GPU to host is done every realization which is
        // inefficient. However this is just done as a proof of concept for
        // having a CPU and a GPU sched (more work necessary for running the 
        // sum on GPU). 
        if (sched == ExecEngineConstants::GPU) {
            JJ.copy_to_host();
        }
        newSum = cv::sum(cv::Mat(h, w, CV_8U, JJ.get()->raw_buffer()->host))[0];
        cout << "new - old: " << newSum << " - " << oldSum << endl;
        // cv::imwrite("out.png", cvJ);
    } while(newSum != oldSum);

    return 0;
}

// // Needs to be static for referencing across mpi processes/nodes
// static struct : RTF::HalGen {
//     std::string getName() {return "stage1_cpu";}
//     int getTarget() {return ExecEngineConstants::CPU;}
//     void realize(std::vector<cv::Mat>& im_ios, 
//                  std::vector<ArgumentBase*>& params) {

//         // Wraps the input and output cv::mat's with halide buffers
//         cv::Mat grayI;
//         cv::cvtColor(im_ios[0], grayI, CV_BGR2GRAY);
//         Halide::Buffer<uint8_t> hI = mat2buf<uint8_t>(&grayI);
//         Halide::Buffer<uint8_t> hJ = mat2buf<uint8_t>(&im_ios[1]);
//         Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[2]);

//         // Define halide stage
//         Halide::Func halCpu;
//         halCpu.define_extern("loopedIwppRecon2", {hI, hJ, hOut, 
//             ExecEngineConstants::CPU}, Halide::UInt(8), 2);

//         // Adds the cpu implementation to the schedules output
//         cout << "[stage1][cpu] Realizing..." << endl;
//         halCpu.realize(hOut);
//         cout << "[stage1][cpu] Done..." << endl;
//     }
// } stage1_cpu;

// // Needs to be static for referencing across mph processes/nodes
// static struct : RTF::HalGen {
//     std::string getName() {return "stage1_gpu";}
//     int getTarget() {return ExecEngineConstants::GPU;}
//     void realize(std::vector<cv::Mat>& im_ios, 
//                  std::vector<ArgumentBase*>& params) {

//         // Wraps the input and output cv::mat's with halide buffers
//         cv::Mat grayI;
//         cv::cvtColor(im_ios[0], grayI, CV_BGR2GRAY);
//         Halide::Buffer<uint8_t> hI = mat2buf<uint8_t>(&grayI);
//         Halide::Buffer<uint8_t> hJ = mat2buf<uint8_t>(&im_ios[1]);
//         Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[2]);
        
//         // Define halide stage
//         Halide::Func halGpu;
//         halGpu.define_extern("loopedIwppRecon2", {hI, hJ, hOut, 
//             ExecEngineConstants::GPU}, Halide::UInt(8), 2);

//         // Adds the cpu implementation to the schedules output
//         cout << "[stage1][gpu] Realizing..." << endl;
//         halGpu.realize(hOut);
//         cout << "[stage1][gpu] Done..." << endl;
//     }
// } stage1_gpu;

// // Needs to be static for referencing across mpi processes/nodes
// static struct : RTF::HalGen {
//     std::string getName() {return "stage2_cpu";}
//     int getTarget() {return ExecEngineConstants::CPU;}
//     void realize(std::vector<cv::Mat>& im_ios, 
//                  std::vector<ArgumentBase*>& params) {

//         // Wraps the input and output cv::mat's with halide buffers
//         Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0]);
//         Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1]);
        
//         // Define halide stage
//         Halide::Func blurx, blury;
//         Halide::Var x, y;
//         Halide::RDom se(-1,3,-1,3);

//         // Generate a bounded hIn for accessing out of border values
//         Halide::Func hbIn = Halide::BoundaryConditions::repeat_edge(hIn);

//         // Implement blur
//         blurx(x,y) = sum(hbIn(x+se.x,y))/3;
//         blury(x,y) = sum(blurx(x,y+se.y))/3;

//         // Perform scheduling
//         blurx.compute_root().parallel(y);
//         blury.compute_root().parallel(x);

//         // Adds the cpu implementation to the schedules output
//         cout << "[stage2][cpu] Realizing..." << endl;
//         blury.realize(hOut);
//         cout << "[stage2][cpu] Done..." << endl;
//     }
// } stage2_cpu;

// // Needs to be static for referencing across mpi processes/nodes
// static struct : RTF::HalGen {
//     std::string getName() {return "stage2_gpu";}
//     int getTarget() {return ExecEngineConstants::GPU;}
//     void realize(std::vector<cv::Mat>& im_ios, 
//                  std::vector<ArgumentBase*>& params) {

//         // Wraps the input and output cv::mat's with halide buffers
//         Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0]);
//         Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1]);
        
//         // Define halide stage
//         Halide::Func blurx, blury;
//         Halide::Var x, y;
//         Halide::RDom se(-1,3,-1,3);

//         // Generate a bounded hIn for accessing out of border values
//         Halide::Func hbIn = Halide::BoundaryConditions::repeat_edge(hIn);

//         // Implement blur
//         blurx(x,y) = sum(hbIn(x+se.x,y))/3;
//         blury(x,y) = sum(blurx(x,y+se.y))/3;

//         // Perform scheduling
//         blurx.compute_root().parallel(y);
//         blury.compute_root().parallel(x);

//         // Adds the cpu implementation to the schedules output
//         cout << "[stage2][gpu] Realizing..." << endl;
//         hIn.set_host_dirty();
//         blury.realize(hOut);
//         hOut.copy_to_host();
//         cout << "[stage2][gpu] Done..." << endl;
//     }
// } stage2_gpu;

// // Explicit registering required since startupSystem called before genStage
// // thus, only the manager node would have the stages registered
// bool r1 = RTF::AutoStage::registerStage(&stage1_cpu);
// bool r2 = RTF::AutoStage::registerStage(&stage1_gpu);
// bool r3 = RTF::AutoStage::registerStage(&stage2_cpu);
// bool r4 = RTF::AutoStage::registerStage(&stage2_gpu);

// Needs to be static for referencing across mpi processes/nodes
static struct : RTF::HalGen {
    std::string getName() {return "get_background";}
    int getTarget() {return ExecEngineConstants::CPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1], "hOut");

        uint8_t blue = ((ArgumentInt*)params[0])->getArgValue();
        uint8_t green = ((ArgumentInt*)params[1])->getArgValue();
        uint8_t red = ((ArgumentInt*)params[2])->getArgValue();

        // Define halide stage
        Halide::Var x, y, c;
        Halide::Func get_bg;

        get_bg(x,y) = 255 * Halide::cast<uint8_t>(
            (hIn(x,y,0) > blue) & (hIn(x,y,1) > green) & (hIn(x,y,2) > red));

        cout << "[get_background][cpu] Realizing..." << endl;
        get_bg.realize(hOut);
        cout << "[get_background][cpu] Done..." << endl;
    }
} get_background;
bool r1 = RTF::AutoStage::registerStage(&get_background);

static struct : RTF::HalGen {
    std::string getName() {return "get_rbc";}
    int getTarget() {return ExecEngineConstants::CPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1], "hOut");

        // Get params
        float T1 = ((ArgumentFloat*)params[0])->getArgValue();
        float T2 = ((ArgumentFloat*)params[1])->getArgValue();

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

        // Realizes bw1 to identify if any pixels were found
        cout << "[get_rbc][cpu] Realizing pre-rbc..." << endl;
        bw1.realize(hbw1);
        int nonz = cv::countNonZero(cvbw1);
        cout << "[get_rbc][cpu] Done pre-rbc..." << endl;

        // Only goes further for the propagation if there is any non-zero pixel
        if (nonz > 0) {
            cout << "[get_rbc][cpu] non-zero: " << nonz << endl;
            // Mat marker = Mat::zeros(bw1.size(), bw1.type());
            // bw2.copyTo(marker, bw1);
            marker(x,y) = Halide::select(bw1(x,y)>0, bw2(x,y), 0);
            // marker(x,y) = Halide::cast<uint8_t>(marker(x,y))*255;

            // bw2.realize(hOut);
            // cv::imwrite("marker.png", im_ios[1]);

            // These two must be scheduled this way since they are inputs
            // for an extern halide function
            marker.compute_root();
            bw2.compute_root();

            // marker = imreconstructBinary<T>(marker, bw2, connectivity);
            rbc.define_extern("loopedIwppRecon", {hmarker, hbw2, 
                ExecEngineConstants::CPU}, Halide::UInt(8), 2);

        } else {
            cout << "[get_rbc][cpu] No propagation" << endl;
            rbc(x,y) = 0;
        }
        
        rbc2(x,y) = rbc(x,y) & marker(x,y) & imR2B(x,y);
        rbc.compute_root();

        cout << "[get_rbc][cpu] Realizing propagation..." << endl;
        rbc2.realize(hOut);
        cout << "[get_rbc][cpu] Done" << endl;
    }
} get_rbc;
bool r2 = RTF::AutoStage::registerStage(&get_rbc);

static struct : RTF::HalGen {
    std::string getName() {return "invert";}
    int getTarget() {return ExecEngineConstants::CPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1], "hOut");

        // Get params
        // channel to be inverted (if -1, then input is grayscale)
        int channel = ((ArgumentInt*)params[0])->getArgValue();

        if (channel == -1)
            assert(im_ios[0].channels() == 2);
        else
            assert(im_ios[0].channels() == 3);

        // Define halide stage
        Halide::Var x, y;
        Halide::Func invert;

        if (channel == -1)
            invert(x,y) = std::numeric_limits<uint8_t>::max()-hIn(x,y);
        else
            invert(x,y) = std::numeric_limits<uint8_t>::max()-hIn(x,y,channel);

        cout << "[invert][cpu] Realizing..." << endl;
        invert.realize(hOut);
        cout << "[invert][cpu] Done..." << endl;
    }
} invert;
bool r3 = RTF::AutoStage::registerStage(&invert);

// Only implemented for grayscale images
// Only implemented for uint8_t images
// Structuring element must have odd width and height
static struct : RTF::HalGen {
    std::string getName() {return "erode";}
    int getTarget() {return ExecEngineConstants::CPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

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

        cout << "[erode][cpu] Realizing..." << endl;
        erode.realize(hOut);
        cout << "[erode][cpu] Done..." << endl;
    }
} erode;
bool r4 = RTF::AutoStage::registerStage(&erode);

static struct : RTF::HalGen {
    std::string getName() {return "dilate";}
    int getTarget() {return ExecEngineConstants::CPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

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
        // mask(x,y) = hSE(x,y)==1 ? hIn(x,y) : 255
        mask(x,y,xi,yi) = hSE(xi,yi)*hIn(x+xi-seWidth,y+yi-seHeight);

        Halide::Expr xc = clamp(x, seWidth, hIn.width()-seWidth);
        Halide::Expr yc = clamp(y, seHeight, hIn.height()-seHeight);
        Halide::RDom se(0, hSE.width()-1, 0, hSE.height()-1);
        dilate(x,y) = maximum(mask(xc,yc,se.x,se.y));

        // Schedules
        Halide::Var pix;
        dilate.fuse(x,y,pix).parallel(pix);

        cout << "[dilate][cpu] Realizing..." << endl;
        dilate.realize(hOut);
        cout << "[dilate][cpu] Done..." << endl;
    }
} dilate;
bool r5 = RTF::AutoStage::registerStage(&dilate);

static struct : RTF::HalGen {
    std::string getName() {return "pre_fill_holes";}
    int getTarget() {return ExecEngineConstants::CPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

        // Wraps the input and output cv::mat's with halide buffers
        Halide::Buffer<uint8_t> hRCOp = mat2buf<uint8_t>(&im_ios[0], "hRCOp");
        Halide::Buffer<uint8_t> hRC = mat2buf<uint8_t>(&im_ios[1], "hRC");
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[2], "hOut");

        // Get params
        int G1 = ((ArgumentInt*)params[0])->getArgValue();

        // sizes must be odd
        assert(im_ios[0].channels() == 1);
        assert(im_ios[1].channels() == 1);

        // Define halide stage
        Halide::Var x, y;
        Halide::Func recon;
        Halide::Func preFill;

        recon.define_extern("loopedIwppRecon", {hRCOp, hRC, 
                this->getTarget()}, Halide::UInt(8), 2);

        preFill(x,y) = Halide::cast<uint8_t>((hRC(x,y) - recon(x,y)) > G1);

        // Schedules
        recon.compute_root();
        preFill.compute_root();
        preFill.parallel(x);

        cout << "[pre_fill_holes][cpu] Realizing..." << endl;
        preFill.realize(hOut);
        cout << "[pre_fill_holes][cpu] Done..." << endl;
    }
} pre_fill_holes;
bool r6 = RTF::AutoStage::registerStage(&pre_fill_holes);

// Needs to be static for referencing across mpi processes/nodes
static struct : RTF::HalGen {
    std::string getName() {return "imreconstruct";}
    int getTarget() {return ExecEngineConstants::CPU;}
    void realize(std::vector<cv::Mat>& im_ios, 
                 std::vector<ArgumentBase*>& params) {

        // Get params
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
        Halide::Buffer<uint8_t> hI = mat2buf<uint8_t>(cvI);
        Halide::Buffer<uint8_t> hJ = mat2buf<uint8_t>(cvJ);
        Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(cvOut);

        // Define halide stage
        Halide::Func halCpu;
        halCpu.define_extern("loopedIwppRecon", {hI, hJ, 
            this->getTarget()}, Halide::UInt(8), 2);

        // Adds the cpu implementation to the schedules output
        cout << "[imreconstruct][cpu] Realizing..." << endl;
        halCpu.realize(hOut);
        cout << "[imreconstruct][cpu] Done..." << endl;
    }
} imreconstruct;
bool r7 = RTF::AutoStage::registerStage(&imreconstruct);

// static struct : RTF::HalGen {
//     std::string getName() {return "fill_holes";}
//     int getTarget() {return ExecEngineConstants::CPU;}
//     void realize(std::vector<cv::Mat>& im_ios, 
//                  std::vector<ArgumentBase*>& params) {

//         // Wraps the input and output cv::mat's with halide buffers
//         // hIn = pre_fill = image
//         Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
//         Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1], "hOut");

//         // input image must be grayscale
//         assert(im_ios[0].channels() == 1);

//         // Creates the marker mat
//         cv::Mat marker(im_ios[0].size(), im_ios[0].type(), 
//             cv::Scalar(std::numeric_limits<uint8_t>::max()));
//     Halide::Buffer<uint8_t> hMarker = mat2buf<uint8_t>(&im_ios[1], "marker");

//         // Define halide stage
//         Halide::Var x, y;
//         Halide::Func invert1("invert1");
//         Halide::Func recon("recon");
//         Halide::Func invert2("invert2");

//         invert1(x,y) = std::numeric_limits<uint8_t>::max() - hIn(x,y);
//         // recon.define_extern("loopedIwppRecon", {hMarker, invert1, 
//         recon.define_extern("loopedIwppRecon", {hIn, hMarker, 
//                 this->getTarget()}, Halide::UInt(8), 2);        
//         invert2(x,y) = std::numeric_limits<uint8_t>::max() - recon(x,y);

// // bw1 = invert(imrec(invert(pre_fill)))

//         // Schedules
//         invert1.compute_root();
//         recon.compute_root();
//         invert2.compute_root();

//         cout << "[fill_holes][cpu] Realizing..." << endl;
//         // invert2.realize(hOut);
//         recon.realize(hOut);
//         // invert1.realize(hOut);
//         cout << "[fill_holes][cpu] Done..." << endl;
//     }
// } fill_holes;
// bool r7 = RTF::AutoStage::registerStage(&fill_holes);

// // 4-conn implementation
// // UNFINISHED!!!
// static struct : RTF::HalGen {
//     std::string getName() {return "bwareaopen2";}
//     int getTarget() {return ExecEngineConstants::CPU;}
//     void realize(std::vector<cv::Mat>& im_ios, 
//                  std::vector<ArgumentBase*>& params) {

//         cv::Mat cvIn;
//         cv::cvtColor(im_ios[0], cvIn, CV_BGR2GRAY);
//         Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&cvIn, "hIn");

//         // Wraps the input and output cv::mat's with halide buffers
//         // Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&im_ios[0], "hIn");
//         Halide::Buffer<uint8_t> hOut = mat2buf<uint8_t>(&im_ios[1], "hOut");

//         // uint8_t minSize = ((ArgumentInt*)params[0])->getArgValue();
//         // uint8_t maxSize = ((ArgumentInt*)params[1])->getArgValue();
//         // uint8_t connectivity = ((ArgumentInt*)params[2])->getArgValue();

//         // Define halide stage
//         Halide::Var x, y;
//         Halide::Buffer<int32_t> label(hIn.width(), hIn.height(), "label");
//         Halide::Func labeli, labelx, labely;
//         Halide::Func area_thr;
//         Halide::Func output;

//         // IWPP initialization
//         labeli(x,y) = x + y * hIn.width();
//         labeli.realize(label);

//         // Clamping label
//         Halide::Expr xc = clamp(x, 1, label.width()-2);
//         Halide::Expr yc = clamp(y, 1, label.height()-2);
//         // Halide::Func clabel;
//         // clabel(x,y) = label(xc,yc);

//         // IWPP propagation conditions with update statements
//         labelx(x,y) = select(hIn(xc,yc) == hIn(xc+1,yc), 
//                              min(label(xc,yc), label(xc+1,yc)),
//                              label(xc,yc));
//         labely(x,y) = select(hIn(xc,yc) == hIn(xc,yc+1), 
//                              min(labelx(xc,yc), labelx(xc,yc+1)),
//                              labelx(xc,yc));

//         // Scheduling for passings
//         labelx.parallel(y);
//         labelx.compute_root();
//         labely.parallel(x);

//         cout << "[bwareaopen2][cpu] Compiling..." << endl;
//         Halide::Target target = Halide::get_host_target();
//         // target.set_feature(Halide::Target::Debug);
//         labely.compile_jit(target);

//         int newSum = 0;
//         int prevSum = 0;
//         do {
//             labely.realize(label);
//             prevSum = newSum;
//             newSum = cv::sum(cv::Mat(label.height(), label.width(), 
//                 CV_8U, label.get()->raw_buffer()->host))[0];
//             cout << "new - prev: " << newSum << " - " << prevSum << endl;
//         } while (prevSum != newSum);

//         // output(x,y) = Halide::cast<uint8_t>(area_thr(x,y) > -1);

//         cout << "[bwareaopen2][cpu] Realizing..." << endl;
//         // output.realize(hOut);
//         cout << "[bwareaopen2][cpu] Done..." << endl;
//     }
// } bwareaopen2;
// bool r6 = RTF::AutoStage::registerStage(&bwareaopen2);

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

    // =========== trying v0.5 === Implementation of the segmentation pipeline
    // =========================== as halide stages (nscale::segmentNuclei:620).
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
    
    // Creates the inputs using RT's autoTiler
    int border = 0;
    int bgThr = 100;
    int erode_param = 4;
    int dilate_param = 10;
    int nTiles = 1;
    BGMasker* bgm = new ThresholdBGMasker(bgThr, dilate_param, erode_param);
    TiledRTCollection* tCollImg = new IrregTiledRTCollection("input", 
        "input", argv[1], border, bgm, 
        NO_PRE_TILER, HBAL_TRIE_QUAD_TREE_ALG, nTiles);

    tCollImg->addImage(argv[1]);
    tCollImg->tileImages();

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
    // RegionTemplate* rtPreRecon = newRT("rtPreRecon");
    // RegionTemplate* rtRecon = newRT("rtRecon");
    // RegionTemplate* rtBw1 = newRT("rtBw1");
    // RegionTemplate* rtFinal = newRT("rtFinal");
    for (int i=0; i<tCollImg->getNumRTs(); i++) {

        // background = get_background(input)
        // bgArea = countNonZero(background) -> exit if ratio > 0.9
        RTF::AutoStage stage0({tCollImg->getRT(i).second, rtBackground}, 
            {new ArgumentInt(blue), new ArgumentInt(green), 
             new ArgumentInt(red)}, {tiles[i].height, tiles[i].width}, 
            {&get_background}, i);
        stage0.genStage(sysEnv);
        
        // rbc = get_rbc(input)
        RTF::AutoStage stage1({tCollImg->getRT(i).second, rtRBC}, 
            {new ArgumentFloat(T1), new ArgumentFloat(T2)}, 
            {tiles[i].height, tiles[i].width}, {&get_rbc}, i);
        stage1.after(&stage0);
        stage1.genStage(sysEnv);

        // rc = invert(input[2])
        RTF::AutoStage stage2({tCollImg->getRT(i).second, rtRC}, 
            {new ArgumentInt(0)}, {tiles[i].height, tiles[i].width}, 
            {&invert}, i);
        stage2.after(&stage1);
        stage2.genStage(sysEnv);
        
        // rc_open = morph_open(rc, disk19raw):
        // rc_open = dilate(erode(rc, disk19raw), disk19raw)
        RTF::AutoStage stage3({rtRC, rtEroded}, 
            {new ArgumentInt(disk19raw_width), 
             new ArgumentIntArray(disk19raw, disk19raw_size)}, 
            {tiles[i].height, tiles[i].width}, {&erode}, i);
        stage3.after(&stage2);
        stage3.genStage(sysEnv);
        RTF::AutoStage stage4({rtEroded, rtRcOpen}, 
            {new ArgumentInt(disk19raw_width), 
             new ArgumentIntArray(disk19raw, disk19raw_size)}, 
            {tiles[i].height, tiles[i].width}, {&dilate}, i);
        stage4.after(&stage3);
        stage4.genStage(sysEnv);

        // RTF::AutoStage stage5({rtRcOpen, rtRC, rtRecon}, 
        //     {new ArgumentInt(-1), new ArgumentInt(1)}, 
        //     {tiles[i].height, tiles[i].width}, {&imreconstruct}, i);
        // stage5.after(&stage4);
        // stage5.genStage(sysEnv);

        // // pre_fill = (rc - imrec(rc_open, rc)) > G1
        // RTF::AutoStage stage6({rtRcOpen, rtRC, rtPreFill}, 
        //     {new ArgumentInt(G1)}, {tiles[i].height, tiles[i].width}, 
        //     {&pre_fill_holes}, i);
        // stage6.after(&stage5);
        // stage6.genStage(sysEnv);






        // // bw1 = fill_holes(pre_fill) = inv(imrec(invert(preFill)))
        // RTF::AutoStage stage6({rtPreFill, rtPreRecon}, {new ArgumentInt(-1)}, 
        //     {tiles[i].height, tiles[i].width}, {&invert}, i);
        // stage6.after(&stage5);
        // stage6.genStage(sysEnv);
        // RTF::AutoStage stage7({rtPreRecon, rtRecon}, 
        //     {new ArgumentInt(-1), new ArgumentInt(1)}, 
        //     {tiles[i].height, tiles[i].width}, {&imreconstruct}, i);
        // stage7.after(&stage6);
        // stage7.genStage(sysEnv);
        // RTF::AutoStage stage8({rtRecon, rtBw1}, {new ArgumentInt(-1)}, 
        //     {tiles[i].height, tiles[i].width}, {&invert}, i);
        // stage8.after(&stage7);
        // stage8.genStage(sysEnv);


        // // bw1_t,compcount2 = bwareaopen2(bw1) //-> exit if compcount2 == 0
        // -=- seg_norbc = bwselect(diffIm > G2, bw1_t) & (rbc == 0)
        // find_cand = seg_norbc

        // RTF::AutoStage stage2({rtPropg, rtBlured}, {}, {tiles[i].height, 
        //     tiles[i].width}, {&stage2_gpu}, i);
        // stage2.after(&stage1);
        // stage2.genStage(sysEnv);
    }

    cout << "started" << endl;
    sysEnv.startupExecution();
    sysEnv.finalizeSystem();

    return 0;
}
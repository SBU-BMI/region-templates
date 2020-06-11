#include "pipeline1.h"

#define PROFILING_STAGES

#define LARGEB

// Only implemented for grayscale images
// Only implemented for uint8_t images
// Structuring element must have odd width and height
void erode(Halide::Buffer<uint8_t> hIn, Halide::Buffer<uint8_t> hOut, 
    Halide::Buffer<uint8_t> hSE, Target_t target, int tileId) {

    #ifdef PROFILING_STAGES
    long st1 = Util::ClockGetTime();
    #endif

    // basic assertions for that ensures that this will work
    assert(hSE.width()%2 != 0);
    assert(hSE.height()%2 != 0);
//    assert(hIn.get().dimensions() == 1);

    int seWidth = (hSE.width()-1)/2;
    int seHeight = (hSE.height()-1)/2;

    // Define halide stage
    Halide::Var x, y, xi, yi, xo, yo;
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
    Halide::Var t;
    Halide::Target hTarget = Halide::get_host_target();
    #ifdef LARGEB
    hTarget.set_feature(Halide::Target::LargeBuffers);
    #endif
    if (target == ExecEngineConstants::CPU) {
        erode.tile(x, y, xo, yo, xi, yi, 16, 16);
        erode.fuse(xo,yo,t).parallel(t);
    } else if (target == ExecEngineConstants::GPU) {
        hTarget.set_feature(Halide::Target::CUDA);
        // hTarget.set_feature(Halide::Target::Debug);
        hIn.set_host_dirty(); // Forcing copy from host to dev
        hOut.set_host_dirty();
        hSE.set_host_dirty();
        erode.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
    }

    string st;
    if (target == ExecEngineConstants::CPU)
        st = "cpu";
    else if (target == ExecEngineConstants::GPU)
        st = "gpu";
    
    // cout << "[erode][" << st << "] Compiling..." << endl;
    erode.compile_jit(hTarget);

    #ifdef PROFILING_STAGES
    long st2 = Util::ClockGetTime();
    cout << "[" << target << "][PROFILING][" << tileId
         << "][STAGE_HAL_COMP][erode] " << (st2-st1) << endl;
    #endif

    // cout << "[erode][" << st << "] Realizing..." << endl;
    erode.realize(hOut);
    if (target == ExecEngineConstants::GPU)
        hOut.copy_to_host();
    // cout << "[erode][" << st << "] Done..." << endl;

    #ifdef PROFILING_STAGES
    long st3 = Util::ClockGetTime();
    cout << "[" << target << "][PROFILING][" << tileId
         << "][STAGE_HAL_EXEC][erode] " << (st3-st2) << endl;
    #endif
}

// Only implemented for grayscale images
// Only implemented for uint8_t images
// Structuring element must have odd width and height
void dilate(Halide::Buffer<uint8_t> hIn, Halide::Buffer<uint8_t> hOut, 
    Halide::Buffer<uint8_t> hSE, Target_t target, int tileId) {

    #ifdef PROFILING_STAGES
    long st0 = Util::ClockGetTime();
    #endif

    // basic assertions for that ensures that this will work
    assert(hSE.width()%2 != 0);
    assert(hSE.height()%2 != 0);
//    assert(im_ios[0].channels() == 1);

    int seWidth = (hSE.width()-1)/2;
    int seHeight = (hSE.height()-1)/2;

    // Define halide stage
    Halide::Var x, y, xi, yi, xo, yo;
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
    Halide::Var t;
    Halide::Target hTarget = Halide::get_host_target();
    #ifdef LARGEB
    hTarget.set_feature(Halide::Target::LargeBuffers);
    #endif
    if (target == ExecEngineConstants::CPU) {
        dilate.tile(x, y, xo, yo, xi, yi, 16, 16);
        dilate.fuse(xo,yo,t).parallel(t);
    } else if (target == ExecEngineConstants::GPU) {
        hTarget.set_feature(Halide::Target::CUDA);
        // hTarget.set_feature(Halide::Target::Debug);
        hIn.set_host_dirty(); // Forcing copy from host to dev
        hOut.set_host_dirty();
        hSE.set_host_dirty();
        dilate.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
    }

    string st;
    if (target == ExecEngineConstants::CPU)
        st = "cpu";
    else if (target == ExecEngineConstants::GPU)
        st = "gpu";

    // cout << "[dilate][" << st << "] Compiling..." << endl;
    dilate.compile_jit(hTarget);

    #ifdef PROFILING_STAGES
    long st2 = Util::ClockGetTime();
    cout << "[" << target << "][PROFILING][" << tileId
         << "][STAGE_HAL_COMP][dilate] " << (st2-st0) << endl;
    #endif

    // cout << "[dilate][" << st << "] Realizing..." << endl;
    dilate.realize(hOut);
    if (target == ExecEngineConstants::GPU)
        hOut.copy_to_host();
    // cout << "[dilate][" << st << "] Done..." << endl;

    #ifdef PROFILING_STAGES
    long st3 = Util::ClockGetTime();
    cout << "[" << target << "][PROFILING][" << tileId
         << "][STAGE_HAL_EXEC][dilate] " << (st3-st2) << endl;
    #endif
}

bool pipeline1(std::vector<cv::Mat>& im_ios, Target_t target, 
             std::vector<ArgumentBase*>& params) {

    // === cv::Mat inputs/outputs =============================================
    // Wraps the input and output cv::mat's with halide buffers
    cv::Mat& cvIn = im_ios[0];
    cv::Mat& cvOut = im_ios[1];
    Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&cvIn, "hIn");
    cv::Mat cvRC(cvOut.rows, cvOut.cols, cvOut.type(), cv::Scalar::all(0));
    Halide::Buffer<uint8_t> hRC = mat2buf<uint8_t>(&cvRC, "hRC");

    // We swap these two outputs on functions which cannot be realized inplace
    cv::Mat cvOut1(cvOut.rows, cvOut.cols, cvOut.type(), cv::Scalar::all(0));
    Halide::Buffer<uint8_t> hOut1 = mat2buf<uint8_t>(&cvOut1, "hOut1");
    Halide::Buffer<uint8_t> hOut2 = mat2buf<uint8_t>(&cvOut, "hOut2");

    // === parameters =========================================================
    int tileId = ((ArgumentInt*)params[0])->getArgValue();

    uint8_t blue = ((ArgumentInt*)params[1])->getArgValue();  // getbg
    uint8_t green = ((ArgumentInt*)params[2])->getArgValue(); // getbg
    uint8_t red = ((ArgumentInt*)params[3])->getArgValue();   // getbg

    // channel to be inverted (if -1, then input is grayscale)
    int channel = ((ArgumentInt*)params[4])->getArgValue();   // invert

    int disk19raw_width = ((ArgumentInt*)params[5])->getArgValue(); // erode/dilate 1
    int* disk19raw = ((ArgumentIntArray*)params[6])->getArgValue(); // erode/dilate 1
    cv::Mat cvSE19(disk19raw_width, disk19raw_width, CV_8U);
    for (int i=0; i<cvSE19.cols; i++) {
        for (int j=0; j<cvSE19.rows; j++) {
            cvSE19.at<uint8_t>(i,j) = disk19raw[i+j*disk19raw_width];
        }
    }
    Halide::Buffer<uint8_t> hSE19 = mat2buf<uint8_t>(&cvSE19, "hSE19"); // erode/dilate 1

    int G1 = ((ArgumentInt*)params[7])->getArgValue(); // preFill

    int disk3raw_width = ((ArgumentInt*)params[8])->getArgValue(); // dilate/erode 2
    int* disk3raw = ((ArgumentIntArray*)params[9])->getArgValue(); // dilate/erode 2
    cv::Mat cvSE3(disk3raw_width, disk3raw_width, CV_8U);
    for (int i=0; i<cvSE3.cols; i++) {
        for (int j=0; j<cvSE3.rows; j++) {
            cvSE3.at<uint8_t>(i,j) = disk3raw[i+j*disk3raw_width];
        }
    }
    Halide::Buffer<uint8_t> hSE3 = mat2buf<uint8_t>(&cvSE3, "hSE3");   // dilate/erode 2

    string st;
    if (target == ExecEngineConstants::CPU)
        st = "cpu";
    else if (target == ExecEngineConstants::GPU)
        st = "gpu";

    Halide::Target hTarget = Halide::get_host_target();
    #ifdef LARGEB
    hTarget.set_feature(Halide::Target::LargeBuffers);
    #endif
    
    // === get-background =================================================
    {
        Halide::Var x, y, c;
        Halide::Func get_bg;
        get_bg(x,y) = 255 * Halide::cast<uint8_t>(
            (hIn(x,y,0) > blue) & (hIn(x,y,1) > green) & (hIn(x,y,2) > red));
        get_bg.compile_jit(hTarget);
        get_bg.realize(hOut1);
    
        long bgArea = cv::countNonZero(cvOut1);
        float ratio = (float)bgArea / (float)(cvOut1.size().area());
    
        // check if there is too much background
        // std::cout << "[get_background][" << tileId << "] ratio: " 
        //     << ratio << std::endl;
        if (ratio >= 0.9) {
            std::cout << "[pipeline1][tile" << tileId 
                << "] aborted!" << std::endl;
            return true; // abort if more than 90% background
        }
    }

    // === invert =========================================================
    {
        if (channel == -1)
            assert(cvIn.channels() == 1);
        else
            assert(cvIn.channels() == 3);

        // Define halide stage
        Halide::Var x, y;
        Halide::Func invert;

        if (channel == -1)
            invert(x,y) = std::numeric_limits<uint8_t>::max()-hIn(x,y);
        else
            invert(x,y) = std::numeric_limits<uint8_t>::max()-hIn(x,y,channel);

        invert.compile_jit(hTarget);
        invert.realize(hRC);
    }

    // === erode/dilate 1 =================================================
    erode(hRC, hOut1, hSE19, target, tileId);
    dilate(hOut1, hOut2, hSE19, target, tileId);

    // === imrecon ========================================================
    loopedIwppRecon(target, cvRC, cvOut);

    // === preFill ========================================================
    {
        // Define halide stage
        Halide::Var x, y;
        Halide::Func preFill;

        preFill(x,y) = 255*Halide::cast<uint8_t>((hRC(x,y) - hOut2(x,y)) > G1);

        // Set the borders as -inf (i.e., 0);
        preFill(x,0) = Halide::cast<uint8_t>(0);
        preFill(x,hOut2.height()-1) = Halide::cast<uint8_t>(0);
        preFill(0,y) = Halide::cast<uint8_t>(0);
        preFill(hOut2.width()-1,y) = Halide::cast<uint8_t>(0);

        // Schedules
        preFill.compute_root();
        Halide::Var t, xo, yo, xi, yi;
        if (target == ExecEngineConstants::CPU) {
            preFill.tile(x, y, xo, yo, xi, yi, 16, 16);
            preFill.fuse(xo,yo,t).parallel(t);
        } else if (target == ExecEngineConstants::GPU) {
            hTarget.set_feature(Halide::Target::CUDA);
            // hTarget.set_feature(Halide::Target::Debug);
            hOut2.set_host_dirty(); // Forcing copy from host to dev
            hRC.set_host_dirty();
            preFill.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
        }
        
        preFill.compile_jit(hTarget);
        preFill.realize(hOut2);
        if (target == ExecEngineConstants::GPU)
            hOut2.copy_to_host();
    }

    // === dilate/erode 2 =================================================
    dilate(hOut2, hOut1, hSE3, target, tileId);
    erode(hOut1, hOut2, hSE3, target, tileId);

    return false;
}

#include "pipeline1.h"

#include <mpi.h>

#define PROFILING_STAGES

#define LARGEB

// // Only used for hybrid testing
// static pthread_barrier_t barrier;
// static bool barried = false;
// static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// Only implemented for grayscale images
// Only implemented for uint8_t images
// Structuring element must have odd width and height
void erode(Halide::Buffer<uint8_t> hIn, Halide::Buffer<uint8_t> hOut,
           Halide::Buffer<uint8_t> hSE, Target_t target, int tileId,
           int noSched, int gpuId = -1) {
    // #ifdef PROFILING_STAGES
    long st1 = Util::ClockGetTime();
    // #endif

    // basic assertions that ensures that this will work
    assert(hSE.width() % 2 != 0);
    assert(hSE.height() % 2 != 0);

    int seWidth  = (hSE.width() - 1) / 2;
    int seHeight = (hSE.height() - 1) / 2;

    // Define halide stage
    Halide::Var  x, y, xi, yi, xo, yo;
    Halide::Func mask;
    Halide::Func erode;

    // Definition of a sub-area on which
    // mask(x,y) = hSE(x,y)==1 ? hIn(x,y) : 255
    mask(x, y, xi, yi) =
        hSE(xi, yi) * hIn(x + xi - seWidth, y + yi - seHeight) +
        (1 - hSE(xi, yi)) * 255;

    Halide::Expr xc = clamp(x, seWidth, hIn.width() - seWidth);
    Halide::Expr yc = clamp(y, seHeight, hIn.height() - seHeight);
    Halide::RDom se(0, hSE.width() - 1, 0, hSE.height() - 1);
    erode(x, y) = minimum(mask(xc, yc, se.x, se.y));

    // Schedules
    Halide::Var    t;
    Halide::Target hTarget = Halide::get_host_target();
#ifdef LARGEB
    hTarget.set_feature(Halide::Target::LargeBuffers);
#endif
    if (target == ExecEngineConstants::CPU && noSched == 0) {
        erode.tile(x, y, xo, yo, xi, yi, 16, 16);
        // erode.fuse(xo,yo,t).parallel(t);
        erode.parallel(yo);
    } else if (target == ExecEngineConstants::GPU) {
        hTarget.set_feature(Halide::Target::CUDA);
        hTarget.set_feature(Halide::Target::Debug);
        erode.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
    }

    string st;
    if (target == ExecEngineConstants::CPU)
        st = "cpu";
    else if (target == ExecEngineConstants::GPU)
        st = "gpu";

    long ct1 = Util::ClockGetTime();
    erode.compile_jit(hTarget);
    long ct2 = Util::ClockGetTime();
    std::cout << "[pipeline1] compiled erode in " << (ct2 - ct1) << " ms"
              << std::endl;

#ifdef PROFILING_STAGES
    long st2 = Util::ClockGetTime();
    cout << "[" << st << "][PROFILING][" << tileId
         << "][STAGE_HAL_COMP][erode] " << (st2 - st1) << endl;
#endif

    if (target == ExecEngineConstants::GPU)
        Halide::Internal::JITSharedRuntime::multigpu_prep_realize(hTarget,
                                                                  gpuId);
    erode.realize(hOut);
    erode.realize(hOut);
    erode.realize(hOut);
    erode.realize(hOut);

    // #ifdef PROFILING_STAGES
    long st3 = Util::ClockGetTime();
    cout << "[pipeline1][" << st << "][PROFILING][" << tileId
         << "][STAGE_HAL_EXEC][erode] " << (st3 - st1) << endl;

    std::cout << "[pipeline1][ptime][tile" << tileId
              << "] erode compile: " << (ct2 - ct1) << " exec: " << (st3 - st2)
              << std::endl;
}

// Only implemented for grayscale images
// Only implemented for uint8_t images
// Structuring element must have odd width and height
void dilate(Halide::Buffer<uint8_t> hIn, Halide::Buffer<uint8_t> hOut,
            Halide::Buffer<uint8_t> hSE, Target_t target, int tileId,
            int noSched, int gpuId = -1) {
    // #ifdef PROFILING_STAGES
    long st0 = Util::ClockGetTime();
    // #endif

    // basic assertions that ensures that this will work
    assert(hSE.width() % 2 != 0);
    assert(hSE.height() % 2 != 0);

    int seWidth  = (hSE.width() - 1) / 2;
    int seHeight = (hSE.height() - 1) / 2;

    // Define halide stage
    Halide::Var  x, y, xi, yi, xo, yo;
    Halide::Func mask;
    Halide::Func dilate;

    // Definition of a sub-area on which
    // mask(x,y) = hSE(x,y)==1 ? hIn(x,y) : 0
    mask(x, y, xi, yi) = hSE(xi, yi) * hIn(x + xi - seWidth, y + yi - seHeight);

    Halide::Expr xc = clamp(x, seWidth, hIn.width() - seWidth);
    Halide::Expr yc = clamp(y, seHeight, hIn.height() - seHeight);
    Halide::RDom se(0, hSE.width() - 1, 0, hSE.height() - 1);
    dilate(x, y) = maximum(mask(xc, yc, se.x, se.y));

    // Schedules
    Halide::Var    t;
    Halide::Target hTarget = Halide::get_host_target();
#ifdef LARGEB
    hTarget.set_feature(Halide::Target::LargeBuffers);
#endif
    if (target == ExecEngineConstants::CPU && noSched == 0) {
        dilate.tile(x, y, xo, yo, xi, yi, 16, 16);
        // dilate.fuse(xo,yo,t).parallel(t);
        dilate.parallel(yo);
    } else if (target == ExecEngineConstants::GPU) {
        hTarget.set_feature(Halide::Target::CUDA);
        hTarget.set_feature(Halide::Target::Debug);
        dilate.gpu_tile(x, y, xo, yo, xi, yi, 16, 16);
    }

    string st;
    if (target == ExecEngineConstants::CPU)
        st = "cpu";
    else if (target == ExecEngineConstants::GPU)
        st = "gpu";

    long ct1 = Util::ClockGetTime();
    dilate.compile_jit(hTarget);
    long ct2 = Util::ClockGetTime();
    std::cout << "[pipeline1] compiled dilate in " << (ct2 - ct1) << " ms"
              << std::endl;

#ifdef PROFILING_STAGES
    long st2 = Util::ClockGetTime();
    cout << "[" << st << "][PROFILING][" << tileId
         << "][STAGE_HAL_COMP][dilate] " << (st2 - st0) << endl;
#endif

    if (target == ExecEngineConstants::GPU)
        Halide::Internal::JITSharedRuntime::multigpu_prep_realize(hTarget,
                                                                  gpuId);
    dilate.realize(hOut);
    dilate.realize(hOut);
    dilate.realize(hOut);
    dilate.realize(hOut);

    // #ifdef PROFILING_STAGES
    long st3 = Util::ClockGetTime();
    cout << "[pipeline1][" << st << "][PROFILING][" << tileId
         << "][STAGE_HAL_EXEC][dilate] " << (st3 - st0) << endl;

    std::cout << "[pipeline1][ptime][tile" << tileId
              << "] dilate compile: " << (ct2 - ct1) << " exec: " << (st3 - st2)
              << std::endl;
}

template <typename T>
Halide::Func halCountNonZero(Halide::Buffer<T> &JJ, Target_t target) {
    // Performs parallel sum on the coordinate with the highest value
    Halide::RDom r({{0, JJ.width()}, {0, JJ.height()}}, "r");
    Halide::Var  x("x");
    Halide::Func cnz("cnz");
    cnz(x) = Halide::cast<long>(0);
    cnz(r.x) += Halide::cast<long>(Halide::select(JJ(r.x, r.y) > 0, 1, 0));

    // Schedule
    Halide::RVar rxo, rxi;
    Halide::Var  xo, xi;
    cnz.update(0).split(r.x, rxo, rxi, 4).reorder(r.y, rxi);
    cnz.update(0).vectorize(rxi).parallel(rxo);

    // Compile
    Halide::Target hTarget = Halide::get_host_target();
#ifdef LARGEB
    hTarget.set_feature(Halide::Target::LargeBuffers);
#endif

    if (target == ExecEngineConstants::GPU) {
        hTarget.set_feature(Halide::Target::CUDA);
    }

    cnz.compile_jit(hTarget);

    // cnz.compile_to_lowered_stmt("cnz.html", {}, Halide::HTML);

    return cnz;
}

bool pipeline1(std::vector<cv::Mat> &im_ios, Target_t target,
               std::vector<ArgumentBase *> &params) {
    // === parameters =========================================================
    int tileId = ((ArgumentInt *)params[0])->getArgValue();

    uint8_t blue  = ((ArgumentInt *)params[1])->getArgValue(); // getbg
    uint8_t green = ((ArgumentInt *)params[2])->getArgValue(); // getbg
    uint8_t red   = ((ArgumentInt *)params[3])->getArgValue(); // getbg

    // channel to be inverted (if -1, then input is grayscale)
    int channel = ((ArgumentInt *)params[4])->getArgValue(); // invert

    int disk19raw_width =
        ((ArgumentInt *)params[5])->getArgValue(); // erode/dilate 1
    int *disk19raw =
        ((ArgumentIntArray *)params[6])->getArgValue(); // erode/dilate 1
    cv::Mat cvHostSE19(disk19raw_width, disk19raw_width, CV_8U);
    for (int i = 0; i < cvHostSE19.cols; i++) {
        for (int j = 0; j < cvHostSE19.rows; j++) {
            cvHostSE19.at<uint8_t>(i, j) = disk19raw[i + j * disk19raw_width];
        }
    }

    int G1 = ((ArgumentInt *)params[7])->getArgValue(); // preFill

    int disk3raw_width =
        ((ArgumentInt *)params[8])->getArgValue(); // dilate/erode 2
    int *disk3raw =
        ((ArgumentIntArray *)params[9])->getArgValue(); // dilate/erode 2
    cv::Mat cvHostSE3(disk3raw_width, disk3raw_width, CV_8U);
    for (int i = 0; i < cvHostSE3.cols; i++) {
        for (int j = 0; j < cvHostSE3.rows; j++) {
            cvHostSE3.at<uint8_t>(i, j) = disk3raw[i + j * disk3raw_width];
        }
    }

    int noSched = ((ArgumentInt *)params[10])->getArgValue(); // halide no sched
    int noIWPP  = ((ArgumentInt *)params[11])->getArgValue(); // no iwpp
    int gpus    = ((ArgumentInt *)params[12])->getArgValue();

    // === cv::Mat inputs/outputs =============================================
    // Wraps the input and output cv::mat's with halide buffers
    cv::Mat &cvHostIn   = im_ios[0];
    cv::Mat &cvHostOut2 = im_ios[1];
    cv::Mat  cvHostRC(cvHostOut2.size(), cvHostOut2.type(), cv::Scalar::all(0));
    cv::Mat  cvHostOut1(cvHostOut2.size(), cvHostOut2.type(),
                        cv::Scalar::all(0));
    cv::Mat  cvHostOut3 = cvHostOut2.clone();

    // === Halide::Target setup ===============================================
    Halide::Target hTarget = Halide::get_host_target();
#ifdef LARGEB
    hTarget.set_feature(Halide::Target::LargeBuffers);
#endif

    // === Halide::Buffer inputs/outputs ======================================
    string st;
    if (target == ExecEngineConstants::CPU) {
        st = "cpu";
    } else if (target == ExecEngineConstants::GPU) {
        st = "gpu";
#ifndef WITH_CUDA
        std::cout << "[pipeline1] Attempted to execute GPU pipeline "
                  << "without CUDA support." << std::endl;
        exit(-1);
#endif
        if (noSched == 1) {
            std::cout << "[pipeline1] Attempted to execute GPU pipeline "
                      << "without halide scheduling (--nhs)." << std::endl;
            exit(-1);
        }
    }

    // Temporary buffers
    Halide::Buffer<uint8_t> hIn = mat2buf<uint8_t>(&cvHostIn, "hIn");
    Halide::Buffer<uint8_t> hRC = mat2buf<uint8_t>(&cvHostRC, "hRC");

    // Disk mats for erosion/dilation
    Halide::Buffer<uint8_t> hSE19 = mat2buf<uint8_t>(&cvHostSE19, "hSE19");
    Halide::Buffer<uint8_t> hSE3  = mat2buf<uint8_t>(&cvHostSE3, "hSE3");

    // We swap these two outputs on functions which cannot be realized inplace
    Halide::Buffer<uint8_t> hOut1 = mat2buf<uint8_t>(&cvHostOut1, "hOut1");
    Halide::Buffer<uint8_t> hOut2 = mat2buf<uint8_t>(&cvHostOut2, "hOut2");
    Halide::Buffer<uint8_t> hOut3 = mat2buf<uint8_t>(&cvHostOut3, "hOut3");

    std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
              << Util::ClockGetTime() << " size: " << cvHostOut1.size()
              << std::endl;
    std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
              << Util::ClockGetTime() << " Executing" << std::endl;

    Halide::Var t, xo, yo, xi, yi;

    // pthread_mutex_lock(&m);
    // if (!barried) {
    //     barried=true;
    //     pthread_barrier_init (&barrier, NULL, 2);
    // }
    // pthread_mutex_unlock(&m);

    // cout << "[pipeline1] waiting " << Util::ClockGetTime() << endl;
    // pthread_barrier_wait (&barrier);
    // cout << "[pipeline1] begin " << Util::ClockGetTime() << endl;

    // === get-background =====================================================
    {
        Halide::Var  x, y, c;
        Halide::Func get_bg;
        get_bg(x, y) = 255 * Halide::cast<uint8_t>((hIn(x, y, 0) > blue) &
                                                   (hIn(x, y, 1) > green) &
                                                   (hIn(x, y, 2) > red));
        long ct1     = Util::ClockGetTime();
        get_bg.compile_jit(hTarget);
        long ct2 = Util::ClockGetTime();
        std::cout << "[pipeline1] compiled getbg in " << (ct2 - ct1) << " ms"
                  << std::endl;
        long rt1 = Util::ClockGetTime();
        get_bg.realize(hOut1);
        long rt2 = Util::ClockGetTime();

        long bgArea = 0;
        // bgArea = cv::countNonZero(cvHostOut1); // returns int: can overflow

        // Performs countNonZero with long return
        Halide::Func lcnz = halCountNonZero(hOut1, ExecEngineConstants::CPU);
        long        *dLineCount = new long[hOut1.width()];
        Halide::Buffer<long> hLineCount =
            Halide::Buffer<long>(dLineCount, hOut1.width(), "hLineCount");
        hLineCount.set_host_dirty();
        long ct3 = Util::ClockGetTime();
        lcnz.compile_jit(hTarget);
        long ct4 = Util::ClockGetTime();
        std::cout << "[pipeline1] compiled contNZ in " << (ct4 - ct3) << " ms"
                  << std::endl;

        long rt3 = Util::ClockGetTime();
        lcnz.realize(hLineCount);
        for (int i = 0; i < hOut1.width(); i++)
            bgArea += dLineCount[i];
        long rt4 = Util::ClockGetTime();

        float ratio = ((long)cvHostOut1.rows) * ((long)cvHostOut1.cols);
        std::cout << "[pipeline1] ratio1: " << ratio << std::endl;
        ratio = bgArea / ratio;
        std::cout << "[pipeline1] ratio2: " << ratio << std::endl;

        // check if there is too much background
        std::cout << "[pipeline1] tile " << tileId << " bgArea: " << bgArea
                  << ", cvSize: " << cvHostOut1.size()
                  << ", cvArea: " << cvHostOut1.size().area() << std::endl;
        std::cout << "[get_background][" << tileId << "] ratio: " << ratio
                  << std::endl;
        if (ratio >= 0.9) {
            std::cout << "[pipeline1][" << st << "][tile" << tileId
                      << "] aborted!" << std::endl;
            // std::cout << "====NOT ABORTING!!!====\n";
            return true; // abort if more than 90% background
        }
        std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
                  << Util::ClockGetTime() << " get_bg done" << std::endl;

        std::cout << "[pipeline1][ptime][tile" << tileId
                  << "] get_bg compile: " << (ct2 - ct1 + ct4 - ct3)
                  << " exec: " << (rt2 - rt1 + rt4 - rt3) << std::endl;
    }

    // === invert =============================================================
    {
        if (channel == -1)
            assert(cvHostIn.channels() == 1);
        else
            assert(cvHostIn.channels() == 3);

        // Define halide stage
        Halide::Var  x, y;
        Halide::Func invert;

        if (channel == -1)
            invert(x, y) = std::numeric_limits<uint8_t>::max() - hIn(x, y);
        else
            invert(x, y) =
                std::numeric_limits<uint8_t>::max() - hIn(x, y, channel);

        // if (target == ExecEngineConstants::GPU) {
        //     hTarget.set_feature(Halide::Target::CUDA);
        // }

        // hIn.set_host_dirty();
        // hRC.set_host_dirty();
        long ct5 = Util::ClockGetTime();
        invert.compile_jit(hTarget);
        long ct6 = Util::ClockGetTime();
        std::cout << "[pipeline1] compiled invert in " << (ct6 - ct5) << " ms"
                  << std::endl;

        // if (target == ExecEngineConstants::GPU)
        //     Halide::Internal::JITSharedRuntime::multigpu_prep_realize(hTarget,
        //     gpuId);

        long rt5 = Util::ClockGetTime();
        invert.realize(hRC);
        long rt6 = Util::ClockGetTime();

        std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
                  << Util::ClockGetTime() << " invert done" << std::endl;

        std::cout << "[pipeline1][ptime][tile" << tileId
                  << "] invert compile: " << (ct6 - ct5)
                  << " exec: " << (rt6 - rt5) << std::endl;
    }

    // Get GPU id for multi-gpu execution
    int gpuId = -1;
    if (target == ExecEngineConstants::GPU) {
        hTarget.set_feature(Halide::Target::CUDA);
        hTarget.set_feature(Halide::Target::Debug);
        gpuId =
            Halide::Internal::JITSharedRuntime::multigpu_setup(hTarget, gpus);
#define MULTI_PROCESS_MULTI_GPU
#ifdef MULTI_PROCESS_MULTI_GPU
        int mpi_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        gpuId = mpi_rank % gpus;
#endif
        std::cout << "[pipeline1][" << st << "][tile" << tileId
                  << "] using GPU " << gpuId << std::endl;
    }

    // === erode/dilate 1 =====================================================
    hRC.set_host_dirty();
    hOut1.set_host_dirty();
    hOut2.set_host_dirty();
    hOut3.set_host_dirty();
    hSE19.set_host_dirty();

    erode(hRC, hOut1, hSE19, target, tileId, noSched, gpuId);
    std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
              << Util::ClockGetTime() << " erode done" << std::endl;
    dilate(hOut1, hOut2, hSE19, target, tileId, noSched, gpuId);
    dilate(hOut1, hOut3, hSE19, target, tileId, noSched, gpuId);
    std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
              << Util::ClockGetTime() << " dilate done" << std::endl;

    if (noIWPP == 0) {
        // === imrecon ========================================================
        int its = loopedIwppRecon(target, hRC, hOut2, noSched, gpuId);
        loopedIwppRecon(target, hRC, hOut3, noSched, gpuId);
        std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
                  << Util::ClockGetTime() << " IWPP with " << its
                  << " iterations" << std::endl;

        // === preFill ========================================================
        {
            // Define halide stage
            Halide::Var  x, y;
            Halide::Func preFill1("preFill1");
            Halide::Func preFill2("preFill2");

            preFill1(x, y) =
                255 * Halide::cast<uint8_t>((hRC(x, y) - hOut2(x, y)) > G1);
            // preFill(x,y) = select((hRC(x,y) - hOut2(x,y)) > G1,
            // Halide::cast<uint8_t>(255), Halide::cast<uint8_t>(0));

            // Set the borders as -inf (i.e., 0);
            preFill2(x, y)                  = Halide::undef<uint8_t>();
            preFill2(x, 0)                  = Halide::cast<uint8_t>(0);
            preFill2(x, hOut2.height() - 1) = Halide::cast<uint8_t>(0);
            preFill2(0, y)                  = Halide::cast<uint8_t>(0);
            preFill2(hOut2.width() - 1, y)  = Halide::cast<uint8_t>(0);

            // Schedules
            if (target == ExecEngineConstants::CPU && noSched == 0) {
                preFill1.parallel(y);
                preFill2.compute_root();
                preFill2.update(0).parallel(x);
                preFill2.update(1).parallel(x);
                preFill2.update(2).parallel(y);
                preFill2.update(3).parallel(y);
            } else if (target == ExecEngineConstants::GPU) {
                hTarget.set_feature(Halide::Target::CUDA);
                hTarget.set_feature(Halide::Target::Debug);
                preFill1.gpu_tile(x, y, xo, yo, xi, yi, 16, 16,
                                  Halide::TailStrategy::GuardWithIf);
                preFill2.update(0).gpu_tile(x, xo, xi, 16,
                                            Halide::TailStrategy::GuardWithIf);
                preFill2.update(1).gpu_tile(x, xo, xi, 16,
                                            Halide::TailStrategy::GuardWithIf);
                preFill2.update(2).gpu_tile(y, yo, yi, 16,
                                            Halide::TailStrategy::GuardWithIf);
                preFill2.update(3).gpu_tile(y, yo, yi, 16,
                                            Halide::TailStrategy::GuardWithIf);
            }

            long ct7 = Util::ClockGetTime();
            preFill1.compile_jit(hTarget);
            preFill2.compile_jit(hTarget);
            // preFill1.compile_to_lowered_stmt("preFill1.html", {},
            // Halide::HTML, hTarget);
            // preFill2.compile_to_lowered_stmt("preFill2.html", {},
            // Halide::HTML, hTarget);
            long ct8 = Util::ClockGetTime();
            std::cout << "[pipeline1] compiled prefill in " << (ct8 - ct7)
                      << " ms" << std::endl;

            if (target == ExecEngineConstants::GPU)
                Halide::Internal::JITSharedRuntime::
                    multigpu_prep_locked_realize(hTarget, gpuId);
            preFill1.realize(hOut2, hTarget);
            // std::cout << "[pipeline1] finished prefill1\n";
            preFill2.realize(hOut2, hTarget);
            if (target == ExecEngineConstants::GPU) {
                Halide::Internal::JITSharedRuntime::multigpu_realized(hTarget);
            }

            std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
                      << Util::ClockGetTime() << " prefill done" << std::endl;
        }

        // === dilate/erode 2 =================================================
        hSE3.set_host_dirty();
        dilate(hOut2, hOut1, hSE3, target, tileId, noSched, gpuId);
        std::cout << "[tapeline][" << st << "][tile" << tileId << "] "
                  << Util::ClockGetTime() << " dilate2 done" << std::endl;
        erode(hOut1, hOut2, hSE3, target, tileId, noSched, gpuId);
        std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
                  << Util::ClockGetTime() << " erode2 done" << std::endl;
    }

    if (target == ExecEngineConstants::GPU) {
        int buffersToFree = 6; // 4 frees and 2 copy
        Halide::Internal::JITSharedRuntime::multigpu_prep_finalize(
            hTarget, gpuId, buffersToFree);
        hOut2.copy_to_host();
        // Halide::Internal::JITSharedRuntime::multigpu_done_finalize(hTarget);
    }

    std::cout << "[pipeline1][" << st << "][tile" << tileId << "] "
              << Util::ClockGetTime() << " Done" << std::endl;

    // cout << "[pipeline1_s] end " << Util::ClockGetTime() << endl;

    return false;
}

bool pipeline1_nscale(std::vector<cv::Mat> &im_ios, Target_t target,
                      std::vector<ArgumentBase *> &params) {
    // === parameters =========================================================
    int tileId = ((ArgumentInt *)params[0])->getArgValue();

    uint8_t blue  = ((ArgumentInt *)params[1])->getArgValue(); // getbg
    uint8_t green = ((ArgumentInt *)params[2])->getArgValue(); // getbg
    uint8_t red   = ((ArgumentInt *)params[3])->getArgValue(); // getbg

    // channel to be inverted (if -1, then input is grayscale)
    int channel = ((ArgumentInt *)params[4])->getArgValue(); // invert

    int disk19raw_width =
        ((ArgumentInt *)params[5])->getArgValue(); // erode/dilate 1
    int *disk19raw =
        ((ArgumentIntArray *)params[6])->getArgValue(); // erode/dilate 1
    cv::Mat cvHostSE19(disk19raw_width, disk19raw_width, CV_8U);
    for (int i = 0; i < cvHostSE19.cols; i++) {
        for (int j = 0; j < cvHostSE19.rows; j++) {
            cvHostSE19.at<uint8_t>(i, j) = disk19raw[i + j * disk19raw_width];
        }
    }

    int G1 = ((ArgumentInt *)params[7])->getArgValue(); // preFill

    int disk3raw_width =
        ((ArgumentInt *)params[8])->getArgValue(); // dilate/erode 2
    int *disk3raw =
        ((ArgumentIntArray *)params[9])->getArgValue(); // dilate/erode 2
    cv::Mat cvHostSE3(disk3raw_width, disk3raw_width, CV_8U);
    for (int i = 0; i < cvHostSE3.cols; i++) {
        for (int j = 0; j < cvHostSE3.rows; j++) {
            cvHostSE3.at<uint8_t>(i, j) = disk3raw[i + j * disk3raw_width];
        }
    }

    string st = "cpu";

    // int noSched = ((ArgumentInt *)params[10])->getArgValue(); // halide no
    // sched int noIWPP = ((ArgumentInt *)params[11])->getArgValue();  // no
    // iwpp int gpus = ((ArgumentInt *)params[12])->getArgValue();

    std::vector<cv::Mat> bgr;
    split(im_ios[0], bgr);

    // === get-background =====================================================
    long    rt1        = Util::ClockGetTime();
    cv::Mat background = (bgr[0] > blue) & (bgr[1] > green) & (bgr[2] > red);
    long    rt2        = Util::ClockGetTime();

    int   bgArea = countNonZero(background);
    float ratio  = ((long)im_ios[0].rows) * ((long)im_ios[0].cols);
    std::cout << "[pipeline1_nscale] ratio1: " << ratio << std::endl;
    ratio = bgArea / ratio;
    std::cout << "[pipeline1_nscale] ratio2: " << ratio << std::endl;

    // check if there is too much background
    std::cout << "[pipeline1_nscale] tile " << tileId << " bgArea: " << bgArea
              << ", cvSize: " << im_ios[0].size()
              << ", cvArea: " << im_ios[0].size().area() << std::endl;
    std::cout << "[get_background][" << tileId << "] ratio: " << ratio
              << std::endl;
    if (ratio >= 0.9) {
        std::cout << "[pipeline1_nscale][" << st << "][tile" << tileId
                  << "] aborted!" << std::endl;
        return true; // abort if more than 90% background
    }
    std::cout << "[pipeline1_nscale][" << st << "][tile" << tileId << "] "
              << Util::ClockGetTime() << " get_bg done" << std::endl;

    std::cout << "[pipeline1_nscale][ptime][tile" << tileId
              << "] get_bg exec: " << (rt2 - rt1) << std::endl;

    // === invert =============================================================
    long    rt3 = Util::ClockGetTime();
    cv::Mat rc;
    CV_Assert(bgr[2].channels() == 1);

    if (std::numeric_limits<unsigned char>::is_integer) {
        if (std::numeric_limits<unsigned char>::is_signed) {
            cv::Mat output;
            bitwise_not(bgr[2], output);
            rc = output + 1;
        } else {
            // unsigned int
            rc = std::numeric_limits<unsigned char>::max() - bgr[2];
        }

    } else {
        // floating point type
        rc = -bgr[2];
    }
    long rt4 = Util::ClockGetTime();

    std::cout << "[pipeline1_nscale][ptime][tile" << tileId
              << "] invert exec: " << (rt4 - rt3) << std::endl;

    // === erode/dilate 1 =====================================================
    long    rt5 = Util::ClockGetTime();
    cv::Mat rc_open(rc.size(), rc.type());

    CV_Assert(cvHostSE19.rows == cvHostSE19.cols);
    CV_Assert(cvHostSE19.rows > 1);
    CV_Assert((cvHostSE19.rows % 2) == 1);

    int bw = (cvHostSE19.rows - 1) / 2;

    // can't use morphologyEx.  the erode phase is not creating a border even
    // though the method signature makes it appear that way. because of this,
    // and the fact that erode and dilate need different border values, have to
    // do the erode and dilate myself.
    //  morphologyEx(image, seg_open, CV_MOP_OPEN, disk3, Point(1,1)); //,
    //  Point(-1, -1), 1, BORDER_REFLECT);
    cv::Mat t_image;

    copyMakeBorder(rc, t_image, bw, bw, bw, bw, cv::BORDER_CONSTANT,
                   std::numeric_limits<unsigned char>::max());
    cv::Mat t_erode = cv::Mat::zeros(t_image.size(), t_image.type());
    erode(t_image, t_erode, cvHostSE19);
    long rt6 = Util::ClockGetTime();

    std::cout << "[pipeline1_nscale][ptime][tile" << tileId
              << "] erode exec: " << (rt6 - rt5) << std::endl;

    long    rt7       = Util::ClockGetTime();
    cv::Mat erode_roi = t_erode(cv::Rect(bw, bw, rc.cols, rc.rows));
    cv::Mat t_erode2;
    copyMakeBorder(erode_roi, t_erode2, bw, bw, bw, bw, cv::BORDER_CONSTANT,
                   std::numeric_limits<unsigned char>::min());
    cv::Mat t_open = cv::Mat::zeros(t_erode2.size(), t_erode2.type());
    dilate(t_erode2, t_open, cvHostSE19);
    cv::Mat open = t_open(cv::Rect(bw, bw, rc.cols, rc.rows));
    long    rt8  = Util::ClockGetTime();

    std::cout << "[pipeline1_nscale][ptime][tile" << tileId
              << "] dilate exec: " << (rt8 - rt7) << std::endl;

    t_open.release();
    t_erode2.release();
    erode_roi.release();
    t_erode.release();

    return false;
}

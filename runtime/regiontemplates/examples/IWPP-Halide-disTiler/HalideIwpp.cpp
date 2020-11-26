#include "HalideIwpp.h"

#define LARGEB

Halide::Func halSumZero(Halide::Target hTarget) {
    Halide::Func pSumZero("pSumZero");
    Halide::Var x("x");
    Halide::Var xo("xo"), xi("xi");

    pSumZero(x) = Halide::cast<long>(0);

    if (hTarget.has_feature(Halide::Target::CUDA)) {
        pSumZero.gpu_tile(x, xo, xi, 10, Halide::TailStrategy::GuardWithIf);
    }

    pSumZero.compile_jit(hTarget);

    return pSumZero;
}

// NEED TO RESET THE INITIAL BUFFER VALUES
template <typename T>
Halide::Func halSum(Halide::Buffer<T>& JJ, Halide::Target hTarget) {
    // Performs parallel sum on the coordinate with the highest value
    Halide::RDom r({{0, JJ.width()}, {0, JJ.height()}}, "r");
    Halide::Var x("x");
    Halide::Func pSum("pSum");
    pSum(x) = Halide::undef<long>();
    pSum(r.x) += Halide::cast<long>(JJ(r.x, r.y));

    // Schedule
    Halide::RVar rxo("rxo"), rxi("rxi");
    Halide::Var xo("xo"), xi("xi");

    if (hTarget.has_feature(Halide::Target::CUDA)) {
        pSum.update()
            .split(r.x, rxo, rxi, 100, Halide::TailStrategy::GuardWithIf)
            .reorder(r.y, rxi, rxo);
        pSum.update().gpu(rxo, rxi);
    } else {
        pSum.update().split(r.x, rxo, rxi, 4).reorder(r.y, rxi, rxo);
        pSum.update().vectorize(rxi).parallel(rxo);
    }

    pSum.compile_jit(hTarget);

    // pSum.compile_to_lowered_stmt("pSum.html", {}, Halide::HTML, hTarget);

    cout << "[halSum] compiled\n";

    return pSum;
}

// It is assumed that the data is already on the proper device/host
// Returns the number of iterations
int loopedIwppRecon(Target_t target, Halide::Buffer<uint8_t>& hI,
                    Halide::Buffer<uint8_t>& hJ, int noSched, int gpuId) {
    // Initial time
    long st0, st1, st2, st3, st4, st5;
    long sti0, sti1, sti2;
    st0 = Util::ClockGetTime();

    // Basic variables for the pipeline
    int32_t w = hI.width();
    int32_t h = hI.height();
    Halide::Func rasterx("rasterx"), arasterx("arasterx");
    Halide::Var x("x"), y("y");
    Halide::RDom prop({{0, w}, {0, h}}, "prop");

    // Raster definition
    using Halide::clamp;
    rasterx(x, y) = Halide::undef<unsigned char>();
    Halide::Expr maxDr = max(rasterx(prop.x, clamp(prop.y - 1, 0, h - 1)),
                             max(rasterx(prop.x, prop.y),
                                 rasterx(clamp(prop.x - 1, 0, w - 1), prop.y)));
    rasterx(prop.x, prop.y) = min(hI(prop.x, prop.y), maxDr);

    // Anti-raster definition
    arasterx(x, y) = Halide::undef<unsigned char>();
    Halide::Expr maxDar =
        max(arasterx(w - prop.x - 1, clamp(h - prop.y, 0, h - 1)),
            max(arasterx(w - prop.x - 1, h - prop.y - 1),
                arasterx(clamp(w - prop.x, 0, w - 1), h - prop.y - 1)));
    arasterx(w - prop.x - 1, h - prop.y - 1) =
        min(hI(w - prop.x - 1, h - prop.y - 1), maxDar);

    // std::cout << "size: " << h << "x" << w << std::endl;

    // Scheduling variables
    Halide::Target hTarget = Halide::get_host_target();
#ifdef LARGEB
    hTarget.set_feature(Halide::Target::LargeBuffers);
#endif
    Halide::RVar rxi("rxi"), rxo("rxo"), ryi("ryi"), ryo("ryo");

    string st;
    if (target == ExecEngineConstants::CPU && noSched == 0) {
        st = "cpu";
        int sFactor =
            h / 56;  // i.e., bridges 28 cores times 2 (for load imbalance)
        // Schedules Raster
        rasterx.update(0).allow_race_conditions();  // for parallel (ryo)
        rasterx.update(0).split(prop.y, ryo, ryi, sFactor,
                                Halide::TailStrategy::GuardWithIf);
        rasterx.update(0).parallel(ryo);

        // Schedules Anti-Raster
        arasterx.update(0).allow_race_conditions();  // for parallel (ryo)
        arasterx.update(0).split(prop.y, ryo, ryi, sFactor,
                                 Halide::TailStrategy::GuardWithIf);
        arasterx.update(0).parallel(ryo);
    } else if (target == ExecEngineConstants::GPU) {
#ifdef WITH_CUDA
        st = "gpu";
        hTarget.set_feature(Halide::Target::CUDA);

        // std::cout << "[" << st << "][IWPP] With CUDA" << std::endl;

        float expected = 1664;  // GTX 970
        // Performs long area multiplication, avoiding overflow
        long area = (long)(h) * (long)(w);
        // minYScanlines must be int for Halide::split()
        int minYScanlines = (float)(area) / expected;
        minYScanlines = sqrt(minYScanlines) * 0.85;
        int minXsize = minYScanlines;  // for reordering
        int threadsSize = 32;          // for no reordering
        // std::cout << "[" << st << "][IWPP] Tile size: "
        //     << minYScanlines << std::endl;

        // Schedules Raster
        rasterx.gpu_blocks(y).gpu_threads(x);
        arasterx.gpu_blocks(y).gpu_threads(x);
        rasterx.update(0).allow_race_conditions();  // for parallel (ryo)
        rasterx.update(0).split(prop.y, ryo, ryi, minYScanlines,
                                Halide::TailStrategy::GuardWithIf);
        arasterx.update(0).allow_race_conditions();  // for parallel (ryo)
        arasterx.update(0).split(prop.y, ryo, ryi, minYScanlines,
                                 Halide::TailStrategy::GuardWithIf);

        rasterx.update(0).split(prop.x, rxo, rxi, minXsize,
                                Halide::TailStrategy::GuardWithIf);
        rasterx.update(0).reorder(rxi, ryi, rxo, ryo);
        rasterx.update(0).gpu_blocks(ryo).gpu_threads(rxo);
        arasterx.update(0).split(prop.x, rxo, rxi, minXsize,
                                 Halide::TailStrategy::GuardWithIf);
        arasterx.update(0).reorder(rxi, ryi, rxo, ryo);
        arasterx.update(0).gpu_blocks(ryo).gpu_threads(rxo);
#else
        std::cout << "[HalideIwpp] Attempted to schedule for GPU without "
                  << "CUDA support." << std::endl;
        exit(-1);
#endif  // if WITH_CUDA
    }

    hTarget.set_feature(Halide::Target::Debug);

    rasterx.compile_jit(hTarget);
    arasterx.compile_jit(hTarget);

    // rasterx.compile_to_lowered_stmt("rasterx.html", {}, Halide::HTML,
    // hTarget); arasterx.compile_to_lowered_stmt("arasterx.html", {},
    // Halide::HTML, hTarget);

    // Halide compilation time
    st1 = Util::ClockGetTime();

    // Sum structures
    unsigned long oldSum = 0;
    unsigned long newSum = 0;
    Halide::Func lsum = halSum(hJ, hTarget);
    long* dLineSum = new long[w];
    Halide::Buffer<long> hLineSum =
        Halide::Buffer<long>(dLineSum, w, "hLineSum");
    Halide::Func lsumzero = halSumZero(hTarget);
    if (target == ExecEngineConstants::GPU) {
        // cout << "[" << st << "][IWPP] hLineSum copying\n";
        hLineSum.set_host_dirty();
        Halide::Internal::JITSharedRuntime::multigpu_prep_finalize(hTarget,
                                                                   gpuId, 1);
        hLineSum.copy_to_device(hTarget);
        // cout << "[" << st << "][IWPP] hLineSum copied\n";
    }

    // Iterate Raster/Anti-Raster until stability
    unsigned long it = 0;
    do {
        cout << "[" << st << "][PROFILING] it: " << it << ", sum = " << newSum
             << std::endl;

        // Initial iteration time
        st2 = Util::ClockGetTime();

        // Realize each raster separately for avoiding allocation of
        // temporary buffers between stages (courtesy of the required
        // compute_root between stages). This is better given that every
        // stage can be updated in-place (i.e., perfect data independence
        // for the opposite coordinate of each raster)
        sti0 = Util::ClockGetTime();
        if (target == ExecEngineConstants::GPU) {
            Halide::Internal::JITSharedRuntime::multigpu_prep_realize(hTarget,
                                                                      gpuId);
        }
        rasterx.realize(hJ);
        sti1 = Util::ClockGetTime();
        if (target == ExecEngineConstants::GPU) {
            Halide::Internal::JITSharedRuntime::multigpu_prep_realize(hTarget,
                                                                      gpuId);
        }
        arasterx.realize(hJ);
        sti2 = Util::ClockGetTime();

        // Raster/Anti-raster time
        st3 = Util::ClockGetTime();

        // cout << "[" << st << "][PROFILING][IWPP_PROP_TIME] "
        //  << (st3-st2) << endl;

        it++;
        oldSum = newSum;

        // Performs parallel sum of the matrix across x on the proper device
        if (target == ExecEngineConstants::GPU) {
            Halide::Internal::JITSharedRuntime::multigpu_prep_realize(hTarget,
                                                                      gpuId);
        }
        lsumzero.realize(hLineSum, hTarget);
        if (target == ExecEngineConstants::GPU) {
            Halide::Internal::JITSharedRuntime::multigpu_prep_realize(hTarget,
                                                                      gpuId);
        }
        lsum.realize(hLineSum, hTarget);

        st4 = Util::ClockGetTime();

        // Performs the host sum after copying from device
        newSum = 0;
        // cout << "[" << st << "][IWPP] cpu sum\n";
        if (target == ExecEngineConstants::GPU) {
            Halide::Internal::JITSharedRuntime::multigpu_prep_finalize(
                hTarget, gpuId, 1);
            hLineSum.copy_to_host();
        }
        for (int i = 0; i < w; i++) newSum += dLineSum[i];

        // Full iteration time
        st5 = Util::ClockGetTime();

        // #ifdef IT_DEBUG
        cout << "[" << st << "][PROFILING][IWPP_SUM_TIME] " << (st5 - st3)
             << endl;
        cout << "[" << st << "][PROFILING][IWPP_FULL_IT_TIME] " << (st5 - st2)
             << std::endl
             << std::endl;
        //#endif

    } while (newSum != oldSum);

    delete[] dLineSum;

    // Clears hLineSum GPU buffer
    if (target == ExecEngineConstants::GPU) {
        Halide::Internal::JITSharedRuntime::multigpu_prep_finalize(hTarget,
                                                                   gpuId, 1);
    }

    // // Final iwpp time
    // long st6 = Util::ClockGetTime();

    // std::cout << "[IWPP][" << st << "][PROFILING] " << it
    //     << " iterations in " << (st6-st0) << " ms" << std::endl;
    // // std::cout << "[" << st << "][PROFILING][IWPP_COMP] "
    // //     << (st1-st0) << std::endl;
    // // std::cout << "[" << st << "][PROFILING][IWPP_EXEC] "
    // //     << (st6-st1) << std::endl;
    // // std::cout << "[" << st << "][PROFILING][IWPP_FULL] "
    // //     << (st6-st0) << std::endl;

    return it;
}

// int loopedIwppRecon(Target_t target, cv::Mat& cvHostI, cv::Mat& cvHostJ)
// {

//     Halide::Buffer<uint8_t> hI;
//     Halide::Buffer<uint8_t> hJ;
//     #ifdef WITH_CUDA
//     cv::cuda::GpuMat cvDevI;
//     cv::cuda::GpuMat* cvDevJ=NULL;
//     #endif // if WITH_CUDA

//     // Initial time
//     long st0, st1, st2, st3, st4, st5;
//     long sti0, sti1, sti2;
//     st0 = Util::ClockGetTime();

//     string st;
//     if (target == ExecEngineConstants::CPU) {
//         st = "cpu";
//         hI = mat2buf<uint8_t>(&cvHostI, "hI");
//         hJ = mat2buf<uint8_t>(&cvHostJ, "hJ");
//     } else if (target == ExecEngineConstants::GPU) {
//         st = "gpu";
//         #ifdef WITH_CUDA
//         Halide::Target hTtarget = Halide::get_host_target();
//         hTtarget.set_feature(Halide::Target::CUDA);

//         // Upload inputs to gpu memory
//         cvDevI.upload(cvHostI);
//         cvDevJ = new cv::cuda::GpuMat();
//         cvDevJ->upload(cvHostJ);

//         // Create halide wrappers for the gpu mat's
//         hI = gpuMat2buf<uint8_t>(cvDevI, hTtarget, "hI");
//         hJ = gpuMat2buf<uint8_t>(*cvDevJ, hTtarget, "hJ1");
//         #else // if not WITH_CUDA
//         std::cout << "No cuda support" << std::endl;
//         exit(-1);
//         #endif // if WITH_CUDA
//     }

//     int it = loopedIwppRecon(target, hI, hJ, cvDevJ);

//     #ifdef WITH_CUDA
//     // Copy result to output
//     if (target == ExecEngineConstants::GPU)
//         cvDevJ->download(cvHostJ);
//     #endif // if WITH_CUDA

//     // Final iwpp time
//     long st6 = Util::ClockGetTime();

//     std::cout << "[IWPP][" << st << "][PROFILING] " << it
//         << " iterations in " << (st6-st0) << " ms" << std::endl;
//     // std::cout << "[" << st << "][PROFILING][IWPP_COMP] "
//     //     << (st1-st0) << std::endl;
//     // std::cout << "[" << st << "][PROFILING][IWPP_EXEC] "
//     //     << (st6-st1) << std::endl;
//     // std::cout << "[" << st << "][PROFILING][IWPP_FULL] "
//     //     << (st6-st0) << std::endl;

//     return 0;
// }

#include "HalideIwpp.h"

#define LARGEB

template <typename T>
Halide::Func halSum2(Halide::Buffer<T> &JJ, Halide::Target hTarget) {
    // Performs parallel sum on the coordinate with the highest value
    // Halide::RDom r({{0, JJ.width()}, {0, JJ.height()}}, "r");
    Halide::Var  x("x"), y("y");
    Halide::RDom rx(0, JJ.width(), "rx");
    Halide::RDom ry(0, JJ.height(), "ry");
    Halide::Func pSum2("pSum2");
    Halide::Func pSum1("pSum1");

    // Zero the first intermediary result array: sum of cols
    pSum2(x) = Halide::cast<long>(0);
    // Perform sum of cols
    pSum2(x) += Halide::cast<long>(JJ(x, ry));

    // // Zero the second intermediary result array: sum of (sum of cols)
    // pSum1() = Halide::cast<long>(0);
    // // Perform sum of sum of cols
    // pSum1() += pSum2(rx);

    // Schedule
    // Halide::RVar rxo("rxo"), rxi("rxi");
    Halide::Var xo("xo"), xi("xi");

    if (hTarget.has_feature(Halide::Target::CUDA)) {
        pSum2.in().compute_root().gpu_tile(x, xo, xi, 100);
        // pSum2.in(pSum1).compute_root().gpu_tile(x, xo, xi, 100);
        // pSum1.in()
        //     .compute_root()
        //     .gpu_single_thread();  // Later rfactor optimize

    } else {
        pSum2.in().split(x, xo, xi, 4).vectorize(xi).parallel(xo);
    }
    // cout << "[halSum] scheduled\n";

    // Halide::Func retFunc(pSum1.in());
    Halide::Func retFunc(pSum2.in());

    retFunc.compile_jit(hTarget);

    // retFunc.compile_to_lowered_stmt("pSum2.html", {}, Halide::HTML, hTarget);

    // cout << "[halSum] compiled\n";

    return retFunc;
}

// Sums a 1D array to a Halide::Buffer scalar
// Maybe use rfactor for associative reduction?
template <typename T>
Halide::Func halSum1(Halide::Buffer<T> &sum2, Halide::Target hTarget) {
    // Performs parallel sum on the coordinate with the highest value
    Halide::RDom r(0, sum2.width(), "r");
    Halide::Func pSum("pSum1");
    pSum() = Halide::cast<long>(0);
    pSum() += Halide::cast<long>(sum2(r));

    // Schedule
    if (hTarget.has_feature(Halide::Target::CUDA)) {
        pSum.in().compute_root().gpu_single_thread();
    }

    pSum.in().compile_jit(hTarget);

    // cout << "compiled halSum1\n";

    // pSum.in().compile_to_lowered_stmt("pSum1.html", {}, Halide::HTML,
    // hTarget);

    return pSum.in();
}

// It is assumed that the data is already on the proper device/host
// Returns the number of iterations
int loopedIwppRecon(Target_t target, Halide::Buffer<uint8_t> &hI,
                    Halide::Buffer<uint8_t> &hJ, int noSched, int gpuId) {
    // Initial time
    long st0, st1, st2, st3, st4, st5;
    long sti0, sti1, sti2;
    st0 = Util::ClockGetTime();

    // Basic variables for the pipeline
    int32_t      w = hI.width();
    int32_t      h = hI.height();
    Halide::Func rasterx("rasterx"), arasterx("arasterx");
    Halide::Var  x("x"), y("y");
    Halide::RDom prop({{0, w}, {0, h}}, "prop");

    // Raster definition
    using Halide::clamp;
    rasterx(x, y)           = Halide::undef<unsigned char>();
    Halide::Expr maxDr      = max(rasterx(prop.x, clamp(prop.y - 1, 0, h - 1)),
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
            h / 56; // i.e., bridges 28 cores times 2 (for load imbalance)
        // Schedules Raster
        rasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        rasterx.update(0).split(prop.y, ryo, ryi, sFactor,
                                Halide::TailStrategy::GuardWithIf);
        rasterx.update(0).parallel(ryo);

        // Schedules Anti-Raster
        arasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        arasterx.update(0).split(prop.y, ryo, ryi, sFactor,
                                 Halide::TailStrategy::GuardWithIf);
        arasterx.update(0).parallel(ryo);
    } else if (target == ExecEngineConstants::GPU) {
#ifdef WITH_CUDA
        st = "gpu";
        hTarget.set_feature(Halide::Target::CUDA);

        // std::cout << "[" << st << "][IWPP] With CUDA" << std::endl;

        float expected = 1664; // GTX 970
        // Performs long area multiplication, avoiding overflow
        long area = (long)(h) * (long)(w);
        // minYScanlines must be int for Halide::split()
        int minYScanlines = (float)(area) / expected;
        minYScanlines     = sqrt(minYScanlines) * 0.85;
        int minXsize      = minYScanlines; // for reordering
        int threadsSize   = 32;            // for no reordering
        // std::cout << "[" << st << "][IWPP] Tile size: "
        //     << minYScanlines << std::endl;

        // Schedules Raster
        rasterx.gpu_blocks(y).gpu_threads(x);
        arasterx.gpu_blocks(y).gpu_threads(x);
        rasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        rasterx.update(0).split(prop.y, ryo, ryi, minYScanlines,
                                Halide::TailStrategy::GuardWithIf);
        arasterx.update(0).allow_race_conditions(); // for parallel (ryo)
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
#endif // if WITH_CUDA
    }

    hTarget.set_feature(Halide::Target::Debug);

    rasterx.compile_jit(hTarget);
    arasterx.compile_jit(hTarget);

    // Halide compilation time
    st1 = Util::ClockGetTime();

    // Sum structures
    long                *dLineSum = new long[w];
    long                 dFullSum;
    Halide::Buffer<long> hLineSum =
        Halide::Buffer<long>(dLineSum, w, "hLineSum");
    Halide::Buffer<long> hFullSum =
        Halide::Buffer<long>::make_scalar(&dFullSum, "hFullSum");

    Halide::Func lsum2 = halSum2(hJ, hTarget);
    Halide::Func lsum1 = halSum1(hLineSum, hTarget);

    // Iterate Raster/Anti-Raster until stability
    unsigned long oldSum = 0;
    unsigned long newSum = 0;
    unsigned long it     = 0;
    do {
        // cout << "[" << st << "][PROFILING] it: " << it << ", sum = " <<
        // newSum << std::endl;

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
        lsum2.realize(hLineSum, hTarget);
        if (target == ExecEngineConstants::GPU) {
            Halide::Internal::JITSharedRuntime::multigpu_prep_realize(hTarget,
                                                                      gpuId);
        }
        lsum1.realize(hFullSum, hTarget);

        st4 = Util::ClockGetTime();

        // Gets the final sum result (from device if the case)
        // cout << "[" << st << "][IWPP] cpu sum\n";
        if (target == ExecEngineConstants::GPU) {
            Halide::Internal::JITSharedRuntime::multigpu_prep_finalize(
                hTarget, gpuId, 1);
            hFullSum.copy_to_host();
        }
        newSum = dFullSum;

        // Full iteration time
        st5 = Util::ClockGetTime();

        // #ifdef IT_DEBUG
        // cout << "[" << st << "][PROFILING][IWPP_SUM_TIME] " << (st5 - st3)
        //      << endl;
        cout << "[" << st << "][PROFILING][IWPP_FULL_IT_TIME][" << it << "] "
             << (st5 - st2) << std::endl; // << std::endl;
                                          //#endif

    } while (newSum != oldSum);

    delete[] dLineSum;

    // Clears hLineSum and hFullSum GPU buffer
    if (target == ExecEngineConstants::GPU) {
        Halide::Internal::JITSharedRuntime::multigpu_prep_finalize(hTarget,
                                                                   gpuId, 2);
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

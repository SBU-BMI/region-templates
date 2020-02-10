#include "HalideIwpp.h"

template <typename T>
Halide::Func halSum(Halide::Buffer<T>& JJ) {

    // Performs parallel sum on the coordinate with the highest value
    Halide::RDom r({{0,JJ.width()-1},{0,JJ.height()-1}}, "r");
    Halide::Var x("x");
    Halide::Func pSum("pSum");
    pSum(x) = Halide::cast<long>(0);//Halide::undef<uint16_t>();
    pSum(r.x) += Halide::cast<long>(JJ(r.x,r.y));

    // Schedule
    Halide::RVar rxo, rxi;
    Halide::Var xo, xi;
    pSum.update(0).split(r.x, rxo, rxi, 4).reorder(r.y, rxi);
    pSum.update(0).vectorize(rxi).parallel(rxo);

    // pSum.compile_to_lowered_stmt("pSum.html", {}, Halide::HTML);

    return pSum;
}

#define PROFILING_STAGES2
template <typename T>
int loopedIwppRecon(IwppExec exOpt, Halide::Buffer<T>& II, Halide::Buffer<T>& JJ,
    Halide::Buffer<T>& hOut) {

    // Initial time
    long st0, st1, st2, st3, st4, st5;
    long sti0, sti1, sti2;
    st0 = Util::ClockGetTime();


    // Basic variables for the pipeline
    int32_t w = II.width();
    int32_t h = II.height();
    Halide::Func rasterx("rasterx"), arasterx("arasterx");
    Halide::Var x("x"), y("y");
    Halide::RDom prop({{1, w-1}, {1, h-1}}, "prop");

    // Raster definition
    rasterx(x,y) = Halide::undef<T>();
    Halide::Expr maxDr = max(rasterx(prop.x,prop.y-1), max(rasterx(prop.x,prop.y), rasterx(prop.x-1,prop.y)));
    rasterx(prop.x,prop.y) = min(II(prop.x,prop.y), maxDr);

    // Anti-raster definition
    arasterx(x,y) = Halide::undef<T>();
    Halide::Expr maxDar = max(arasterx(w-prop.x-1,h-prop.y), max(arasterx(w-prop.x-1,h-prop.y-1), arasterx(w-prop.x,h-prop.y-1)));
    arasterx(w-prop.x-1,h-prop.y-1) = min(II(w-prop.x-1,h-prop.y-1), maxDar);


    // Scheduling variables
    Halide::Target target = Halide::get_host_target();
    Halide::RVar ryi("ryi"), ryo("ryo");
    
    int sFactor = h/56; // i.e., bridges 28 cores times 2 (for load imbalance)

    if (exOpt == CPU) {
        // Schedules Raster
        rasterx.update(0).split(prop.y, ryo, ryi, sFactor);
        rasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        rasterx.update(0).parallel(ryo);
        rasterx.compile_jit();

        // Schedules Anti-Raster
        arasterx.update(0).split(prop.y, ryo, ryi, sFactor);
        arasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        arasterx.update(0).parallel(ryo);
        arasterx.compile_jit();
    } else if (exOpt == GPU) {
        #ifdef WITH_CUDA

        #else
        std::cout << "[HalideIwpp] Attempted to schedule for GPU without "
            << "CUDA support." << std::endl;
        exit(-1);
        #endif
    }


        // } else if (sched == ExecEngineConstants::GPU) {
        //     Halide::Var bx, by, tx, ty;
        //     rasterx.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        //     rastery.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        //     target.set_feature(Halide::Target::CUDA);
        //     II.set_host_dirty();
        //     JJ.set_host_dirty();
        // }

    // rasterx.compile_to_lowered_stmt("rasterx.html", {}, Halide::HTML);

    // Halide compilation time
    st1 = Util::ClockGetTime();

    // Sum structures
    unsigned long oldSum = 0;
    unsigned long newSum = 0;
    Halide::Func lsum = halSum(JJ);
    long* dLineSum = new long[w];
    Halide::Buffer<long> hLineSum = Halide::Buffer<long>(dLineSum, w, "hLineSum");

    // Iterate Raster/Anti-Raster until stability
    unsigned long it = 0;
    do {
        // Initial iteration time
        st2 = Util::ClockGetTime();

        // Realize each raster separately for avoiding allocation of temporary buffers
        // between stages (courtesy of the required compute_root between stages)
        // This is better given that every stage can be updated in-place (i.e., perfect
        // data independence for the opposite coordinate of each raster)
        sti0 = Util::ClockGetTime();
        rasterx.realize(JJ);
        sti1 = Util::ClockGetTime();
        arasterx.realize(JJ);
        sti2 = Util::ClockGetTime();

        // // Copy from GPU to host is done every realization which is
        // // inefficient. However this is just done as a proof of concept for
        // // having a CPU and a GPU sched (more work necessary for running the 
        // // sum on GPU). 
        // if (sched == ExecEngineConstants::GPU) {
        //     JJ.copy_to_host();
        // }

        // Raster/Anti-raster time
        st3 = Util::ClockGetTime();

        it++;
        oldSum = newSum;

        // Performs parallel sum of the matrix across x, then sequentially
        // sums the result
        lsum.realize(hLineSum);
        st4 = Util::ClockGetTime();
        newSum = 0;
        for (int i=0; i<w; i++)
            newSum += dLineSum[i];

        // Full iteration time
        st5 = Util::ClockGetTime();
        
        #ifdef IT_DEBUG
        cout << "[PROFILING][IWPP_PROP_TIME] " << (st3-st2) << endl;
        cout << "[PROFILING][IWPP_SUM_TIME] h=" << (st4-st3) 
             << ", s=" << (st5-st4) << endl;
        cout << "[PROFILING][IWPP_FULL_IT_TIME] " << (st5-st2) << std::endl;
        #endif

    } while(newSum != oldSum);

    delete[] dLineSum;

    // Copy result to output
    hOut.copy_from(JJ); // bad

    // Final iwpp time
    long st6 = Util::ClockGetTime();

    std::cout << "[PROFILING][IWPP] iterations: " << it << std::endl;
    std::cout << "[PROFILING][IWPP_COMP] " << (st1-st0) << std::endl;
    std::cout << "[PROFILING][IWPP_EXEC] " << (st6-st1) << std::endl;
    std::cout << "[PROFILING][IWPP_FULL] " << (st6-st0) << std::endl;

    return 0;
}

template <typename T>
int loopedIwppReconGPU(IwppExec exOpt, Halide::Buffer<T>& II, Halide::Buffer<T>& JJ,
    Halide::Buffer<T>& hOut) {

    // Initial time
    long st0, st1, st2, st3, st4, st5;
    long sti0, sti1, sti2;
    st0 = Util::ClockGetTime();


    // Basic variables for the pipeline
    int32_t w = II.width();
    int32_t h = II.height();
    Halide::Func rasterx("rasterx"), arasterx("arasterx");
    Halide::Var x("x"), y("y");
    Halide::RDom prop({{1, w-1}, {1, h-1}}, "prop");

    // Raster definition
    rasterx(x,y) = Halide::undef<T>();
    Halide::Expr maxDr = max(rasterx(prop.x,prop.y-1), max(rasterx(prop.x,prop.y), rasterx(prop.x-1,prop.y)));
    rasterx(prop.x,prop.y) = min(II(prop.x,prop.y), maxDr);

    // Anti-raster definition
    arasterx(x,y) = Halide::undef<T>();
    Halide::Expr maxDar = max(arasterx(w-prop.x-1,h-prop.y), max(arasterx(w-prop.x-1,h-prop.y-1), arasterx(w-prop.x,h-prop.y-1)));
    arasterx(w-prop.x-1,h-prop.y-1) = min(II(w-prop.x-1,h-prop.y-1), maxDar);


    // Scheduling variables
    Halide::Target target = Halide::get_host_target();
    Halide::RVar ryi("ryi"), ryo("ryo");
    
    int sFactor = h/56; // i.e., bridges 28 cores times 2 (for load imbalance)

    if (exOpt == CPU) {
        // Schedules Raster
        rasterx.update(0).split(prop.y, ryo, ryi, sFactor);
        rasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        rasterx.update(0).parallel(ryo);
        rasterx.compile_jit();

        // Schedules Anti-Raster
        arasterx.update(0).split(prop.y, ryo, ryi, sFactor);
        arasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        arasterx.update(0).parallel(ryo);
        arasterx.compile_jit();
    } else if (exOpt == GPU) {
        #ifdef WITH_CUDA
        
        #else
        std::cout << "[HalideIwpp] Attempted to schedule for GPU without "
            << "CUDA support." << std::endl;
        exit(-1);
        #endif
    }


        // } else if (sched == ExecEngineConstants::GPU) {
        //     Halide::Var bx, by, tx, ty;
        //     rasterx.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        //     rastery.gpu_tile(x, y, bx, by, tx, ty, 16, 16);
        //     target.set_feature(Halide::Target::CUDA);
        //     II.set_host_dirty();
        //     JJ.set_host_dirty();
        // }

    // rasterx.compile_to_lowered_stmt("rasterx.html", {}, Halide::HTML);

    // Halide compilation time
    st1 = Util::ClockGetTime();

    // Sum structures
    unsigned long oldSum = 0;
    unsigned long newSum = 0;
    Halide::Func lsum = halSum(JJ);
    long* dLineSum = new long[w];
    Halide::Buffer<long> hLineSum = Halide::Buffer<long>(dLineSum, w, "hLineSum");

    // Iterate Raster/Anti-Raster until stability
    unsigned long it = 0;
    do {
        // Initial iteration time
        st2 = Util::ClockGetTime();

        // Realize each raster separately for avoiding allocation of temporary buffers
        // between stages (courtesy of the required compute_root between stages)
        // This is better given that every stage can be updated in-place (i.e., perfect
        // data independence for the opposite coordinate of each raster)
        sti0 = Util::ClockGetTime();
        rasterx.realize(JJ);
        sti1 = Util::ClockGetTime();
        arasterx.realize(JJ);
        sti2 = Util::ClockGetTime();

        // // Copy from GPU to host is done every realization which is
        // // inefficient. However this is just done as a proof of concept for
        // // having a CPU and a GPU sched (more work necessary for running the 
        // // sum on GPU). 
        // if (sched == ExecEngineConstants::GPU) {
        //     JJ.copy_to_host();
        // }

        // Raster/Anti-raster time
        st3 = Util::ClockGetTime();

        it++;
        oldSum = newSum;

        // Performs parallel sum of the matrix across x, then sequentially
        // sums the result
        lsum.realize(hLineSum);
        st4 = Util::ClockGetTime();
        newSum = 0;
        for (int i=0; i<w; i++)
            newSum += dLineSum[i];

        // Full iteration time
        st5 = Util::ClockGetTime();
        
        #ifdef IT_DEBUG
        cout << "[PROFILING][IWPP_PROP_TIME] " << (st3-st2) << endl;
        cout << "[PROFILING][IWPP_SUM_TIME] h=" << (st4-st3) 
             << ", s=" << (st5-st4) << endl;
        cout << "[PROFILING][IWPP_FULL_IT_TIME] " << (st5-st2) << std::endl;
        #endif

    } while(newSum != oldSum);

    delete[] dLineSum;

    // Copy result to output
    hOut.copy_from(JJ); // bad

    // Final iwpp time
    long st6 = Util::ClockGetTime();

    std::cout << "[PROFILING][IWPP] iterations: " << it << std::endl;
    std::cout << "[PROFILING][IWPP_COMP] " << (st1-st0) << std::endl;
    std::cout << "[PROFILING][IWPP_EXEC] " << (st6-st1) << std::endl;
    std::cout << "[PROFILING][IWPP_FULL] " << (st6-st0) << std::endl;

    return 0;
}


template int loopedIwppRecon(IwppExec exOpt, Halide::Buffer<unsigned char>& II, 
    Halide::Buffer<unsigned char>& JJ, Halide::Buffer<unsigned char>& hOut);
template int loopedIwppReconGPU(IwppExec exOpt, Halide::Buffer<unsigned char>& II, 
    Halide::Buffer<unsigned char>& JJ, Halide::Buffer<unsigned char>& hOut);
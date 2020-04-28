#include "HalideIwpp.h"

template <typename T>
Halide::Func halSum(Halide::Buffer<T>& JJ) {

    // Performs parallel sum on the coordinate with the highest value
    Halide::RDom r({{0,JJ.width()},{0,JJ.height()}}, "r");
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

int loopedIwppRecon(Target_t target, cv::Mat& cvHostI, cv::Mat& cvHostJ) {

    Halide::Buffer<uint8_t> hI;
    Halide::Buffer<uint8_t> hJ;
    #ifdef WITH_CUDA
    cv::cuda::GpuMat cvDevI, cvDevJ;
    #endif // if WITH_CUDA

    if (target == ExecEngineConstants::CPU) {
        hI = mat2buf<uint8_t>(&cvHostI, "hI");
        hJ = mat2buf<uint8_t>(&cvHostJ, "hJ");
    } else if (target == ExecEngineConstants::GPU) {
        #ifdef WITH_CUDA
        Halide::Target hTtarget = Halide::get_host_target();
        hTtarget.set_feature(Halide::Target::CUDA);

        // Upload inputs to gpu memory
        cvDevI.upload(cvHostI);
        cvDevJ.upload(cvHostJ);

        // Create halide wrappers for the gpu mat's
        hI = gpuMat2buf<uint8_t>(cvDevI, hTtarget, "hI");
        hJ = gpuMat2buf<uint8_t>(cvDevJ, hTtarget, "hJ1");
        #else // if not WITH_CUDA
        std::cout << "No cuda support" << std::endl;
        exit(-1);
        #endif // if WITH_CUDA
    }

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
    rasterx(x,y) = Halide::undef<unsigned char>();
    Halide::Expr maxDr = max(rasterx(prop.x,clamp(prop.y-1,0,h-1)), 
                         max(rasterx(prop.x,prop.y), 
                             rasterx(clamp(prop.x-1, 0, w-1),prop.y)));
    rasterx(prop.x,prop.y) = min(hI(prop.x,prop.y), maxDr);

    // Anti-raster definition
    arasterx(x,y) = Halide::undef<unsigned char>();
    Halide::Expr maxDar = max(arasterx(w-prop.x-1,clamp(h-prop.y,0,h-1)), 
                          max(arasterx(w-prop.x-1,h-prop.y-1), 
                              arasterx(clamp(w-prop.x,0,w-1),h-prop.y-1)));
    arasterx(w-prop.x-1,h-prop.y-1) = min(hI(w-prop.x-1,h-prop.y-1), maxDar);

    // std::cout << "size: " << h << "x" << w << std::endl;

    // Scheduling variables
    Halide::Target hTarget = Halide::get_host_target();
    Halide::RVar rxi("rxi"), rxo("rxo"), ryi("ryi"), ryo("ryo");
    
    if (target == ExecEngineConstants::CPU) {
        int sFactor = h/56; // i.e., bridges 28 cores times 2 (for load imbalance)
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
        hTarget.set_feature(Halide::Target::CUDA);

        std::cout << "[" << target << "][IWPP] With CUDA" << std::endl;

        int expected = 1664; // GTX 970
        int minYScanlines = sqrt(h*w/expected) * 0.85;
        int minXsize = minYScanlines; // for reordering
        int threadsSize = 32; // for no reordering
        std::cout << "[" << target << "][IWPP] Tile size: " 
            << minYScanlines << std::endl;
        
        // Schedules Raster
        rasterx.gpu_blocks(y).gpu_threads(x);
        arasterx.gpu_blocks(y).gpu_threads(x);
        rasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        rasterx.update(0).split(prop.y, ryo, ryi, minYScanlines, 
            Halide::TailStrategy::GuardWithIf);
        arasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        arasterx.update(0).split(prop.y, ryo, ryi, minYScanlines, 
            Halide::TailStrategy::GuardWithIf);

        std::cout << "[" << target << "][IWPP] With reorder" << std::endl;
        rasterx.update(0).split(prop.x, rxo, rxi, minXsize, 
            Halide::TailStrategy::GuardWithIf);
        rasterx.update(0).reorder(rxi,ryi,rxo,ryo);
        rasterx.update(0).gpu_blocks(ryo).gpu_threads(rxo);
        arasterx.update(0).split(prop.x, rxo, rxi, minXsize, 
            Halide::TailStrategy::GuardWithIf);
        arasterx.update(0).reorder(rxi,ryi,rxo,ryo);
        arasterx.update(0).gpu_blocks(ryo).gpu_threads(rxo);
        #else
        std::cout << "[HalideIwpp] Attempted to schedule for GPU without "
            << "CUDA support." << std::endl;
        exit(-1);
        #endif // if WITH_CUDA
    }

    rasterx.compile_jit(hTarget);
    arasterx.compile_jit(hTarget);

    //rasterx.compile_to_lowered_stmt("rasterx.html", {}, Halide::HTML, hTarget);
    //arasterx.compile_to_lowered_stmt("arasterx.html", {}, Halide::HTML, hTarget);

    // Halide compilation time
    st1 = Util::ClockGetTime();

    // Sum structures
    unsigned long oldSum = 0;
    unsigned long newSum = 0;
    Halide::Func lsum = halSum(hJ);
    long* dLineSum = new long[w];
    Halide::Buffer<long> hLineSum = Halide::Buffer<long>(dLineSum, w, "hLineSum");

    // Iterate Raster/Anti-Raster until stability
    unsigned long it = 0;
    do {
        // Initial iteration time
        st2 = Util::ClockGetTime();

        // Realize each raster separately for avoiding allocation of temporary 
        // buffers between stages (courtesy of the required compute_root between 
        // stages). This is better given that every stage can be updated in-place 
        // (i.e., perfect data independence for the opposite coordinate of 
        // each raster)
        sti0 = Util::ClockGetTime();
        rasterx.realize(hJ);
        sti1 = Util::ClockGetTime();
        arasterx.realize(hJ);
        sti2 = Util::ClockGetTime();

        // Raster/Anti-raster time
        st3 = Util::ClockGetTime();

        it++;
        oldSum = newSum;

        if (target == ExecEngineConstants::CPU) {
            // Performs parallel sum of the matrix across x, then sequentially
            // sums the result
            lsum.realize(hLineSum);
            st4 = Util::ClockGetTime();
            newSum = 0;
            for (int i=0; i<w; i++)
                newSum += dLineSum[i];
        } else if (target == ExecEngineConstants::GPU) {
            #ifdef WITH_CUDA
            newSum = cv::cuda::sum(cvDevJ)[0];
            #endif // if WITH_CUDA
        }

        // Full iteration time
        st5 = Util::ClockGetTime();
        
        #ifdef IT_DEBUG
        cout << "[" << target << "][PROFILING] it: " << it << ", sum = " 
            << newSum << std::endl;
        cout << "[" << target << "][PROFILING][IWPP_PROP_TIME] " 
            << (st3-st2) << endl;
        cout << "[" << target << "][PROFILING][IWPP_SUM_TIME] " 
            << (st5-st3) << endl;
        cout << "[" << target << "][PROFILING][IWPP_FULL_IT_TIME] " 
            << (st5-st2) << std::endl;
        #endif

    } while(newSum != oldSum);

    delete[] dLineSum;

    #ifdef WITH_CUDA
    // Copy result to output
    if (target == ExecEngineConstants::GPU)
        cvDevJ.download(cvHostJ);
    #endif // if WITH_CUDA

    // Final iwpp time
    long st6 = Util::ClockGetTime();

    std::cout << "[" << target << "][PROFILING][IWPP] iterations: " 
        << it << std::endl;
    std::cout << "[" << target << "][PROFILING][IWPP_COMP] " 
        << (st1-st0) << std::endl;
    std::cout << "[" << target << "][PROFILING][IWPP_EXEC] " 
        << (st6-st1) << std::endl;
    std::cout << "[" << target << "][PROFILING][IWPP_FULL] " 
        << (st6-st0) << std::endl;

    return 0;
}

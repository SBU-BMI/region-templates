#include "HalideIwpp.h"

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

// This manual setup is required since GPU data may have a different
// memory displacement than on host memory. Tldr, need to fix
// the stride of Halide::Buffer for data already on GPU.
template <typename T>
Halide::Buffer<T> gpuMat2buf(cv::cuda::GpuMat& m, Halide::Target& t, 
        std::string name="") {

    //int extents[] = {m.cols, m.rows};
    //int strides[] = {1, ((int)m.step)/((int)sizeof(T))};
    //int mins[] = {0, 0};
    buffer_t devB = {0, NULL, {m.cols, m.rows}, 
                     {1, ((int)m.step)/((int)sizeof(T))}, 
                     {0, 0}, sizeof(T)};

    
    Halide::Buffer<T> hDev = name.empty() ? 
        Halide::Buffer<T>(devB) : Halide::Buffer<T>(devB, name);
    hDev.device_wrap_native(Halide::DeviceAPI::CUDA, (intptr_t)m.data, t);

    return hDev;
}

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
    Halide::Buffer<T>& hOut, cv::cuda::GpuMat& cvDevJ, cv::Mat& cvHostOut) {

    // Initial time
    long st0, st1, st2, st3, st4, st5;
    long sti0, sti1, sti2;
    st0 = Util::ClockGetTime();


    // Basic variables for the pipeline
    int32_t w = II.width();
    int32_t h = II.height();
    Halide::Func rasterx("rasterx"), arasterx("arasterx");
    Halide::Var x("x"), y("y");
    Halide::RDom prop({{0, w}, {0, h}}, "prop");

    // Raster definition
    using Halide::clamp;
    rasterx(x,y) = Halide::undef<T>();
    Halide::Expr maxDr = max(rasterx(prop.x,clamp(prop.y-1,0,h-1)), 
                         max(rasterx(prop.x,prop.y), 
                             rasterx(clamp(prop.x-1, 0, w-1),prop.y)));
    rasterx(prop.x,prop.y) = min(II(prop.x,prop.y), maxDr);

    // Anti-raster definition
    arasterx(x,y) = Halide::undef<T>();
    Halide::Expr maxDar = max(arasterx(w-prop.x-1,clamp(h-prop.y,0,h-1)), 
                          max(arasterx(w-prop.x-1,h-prop.y-1), 
                              arasterx(clamp(w-prop.x,0,w-1),h-prop.y-1)));
    arasterx(w-prop.x-1,h-prop.y-1) = min(II(w-prop.x-1,h-prop.y-1), maxDar);

    std::cout << "size: " << h << "x" << w << std::endl;


    // Scheduling variables
    Halide::Target target = Halide::get_host_target();
    Halide::RVar rxi("rxi"), rxo("rxo"), ryi("ryi"), ryo("ryo");
    
    int sFactor = h/56; // i.e., bridges 28 cores times 2 (for load imbalance)

    if (exOpt == CPU || exOpt == CPU_REORDER) {
        // Schedules Raster
        rasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        rasterx.update(0).split(prop.y, ryo, ryi, sFactor);
        rasterx.update(0).parallel(ryo);
        if (exOpt == CPU_REORDER) {
            rasterx.update(0).split(prop.x, rxo, rxi, w/4);
            rasterx.update(0).reorder(rxi,ryi,rxo,ryo);
            rasterx.update(0).vectorize(rxo);
        }

        // Schedules Anti-Raster
        arasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        arasterx.update(0).split(prop.y, ryo, ryi, sFactor);
        arasterx.update(0).parallel(ryo);
        if (exOpt == CPU_REORDER) {
            arasterx.update(0).split(prop.x, rxo, rxi, w/4);
            arasterx.update(0).reorder(rxi,ryi,rxo,ryo);
            arasterx.update(0).vectorize(rxo);
        }
    } else if (exOpt == GPU || GPU_REORDER) {
        #ifdef WITH_CUDA
        target = Halide::get_host_target();
        target.set_feature(Halide::Target::CUDA);

        std::cout << "[IWPP] With CUDA" << std::endl;

        int minYScanlines = 512;
        int minXsize = 512;
        int threadsSize = 32;
        
        // Schedules Raster
        rasterx.gpu_blocks(y).gpu_threads(x);
        arasterx.gpu_blocks(y).gpu_threads(x);
        rasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        rasterx.update(0).split(prop.y, ryo, ryi, minYScanlines);
        arasterx.update(0).allow_race_conditions(); // for parallel (ryo)
        arasterx.update(0).split(prop.y, ryo, ryi, minYScanlines);
        if (exOpt == GPU_REORDER) {
            std::cout << "[IWPP] With reorder" << std::endl;
            rasterx.update(0).split(prop.x, rxo, rxi, minXsize);
            rasterx.update(0).reorder(rxi,ryi,rxo,ryo);
            rasterx.update(0).gpu_blocks(ryo).gpu_threads(rxo);
            arasterx.update(0).split(prop.x, rxo, rxi, minXsize);
            arasterx.update(0).reorder(rxi,ryi,rxo,ryo);
            arasterx.update(0).gpu_blocks(ryo).gpu_threads(rxo);
        } else {
            Halide::RVar b("b"), t("t");
            rasterx.update(0).split(ryo, b, t, threadsSize);
            rasterx.update(0).gpu_blocks(b).gpu_threads(t);
            arasterx.update(0).split(ryo, b, t, threadsSize);
            arasterx.update(0).gpu_blocks(b).gpu_threads(t);
        }
        #else
        std::cout << "[HalideIwpp] Attempted to schedule for GPU without "
            << "CUDA support." << std::endl;
        exit(-1);
        #endif
    }

    rasterx.compile_jit(target);
    arasterx.compile_jit(target);

    //rasterx.compile_to_lowered_stmt("rasterx.html", {}, Halide::HTML, target);
    //arasterx.compile_to_lowered_stmt("arasterx.html", {}, Halide::HTML, target);

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

        // Raster/Anti-raster time
        st3 = Util::ClockGetTime();

        it++;
        oldSum = newSum;

        if (exOpt == CPU || exOpt == CPU_REORDER) {
            // Performs parallel sum of the matrix across x, then sequentially
            // sums the result
            lsum.realize(hLineSum);
            st4 = Util::ClockGetTime();
            newSum = 0;
            for (int i=0; i<w; i++)
                newSum += dLineSum[i];
        } else if (exOpt == GPU || exOpt == GPU_REORDER) {
            #ifdef WITH_CUDA
            newSum = cv::cuda::sum(cvDevJ)[0];
            #endif
        }

        // Full iteration time
        st5 = Util::ClockGetTime();
        
        //#ifdef IT_DEBUG
        cout << "[PROFILING] it: " << it << ", sum = " << newSum << std::endl;
        cout << "[PROFILING][IWPP_PROP_TIME] " << (st3-st2) << endl;
        cout << "[PROFILING][IWPP_SUM_TIME] " << (st5-st3) << endl;
        cout << "[PROFILING][IWPP_FULL_IT_TIME] " << (st5-st2) << std::endl;
        //#endif

    } while(newSum != oldSum);

    delete[] dLineSum;

    // Copy result to output
    if (exOpt == CPU || exOpt == CPU_REORDER)
        hOut.copy_from(JJ); // bad
    else if (exOpt == GPU || exOpt == GPU_REORDER)
        cvDevJ.download(cvHostOut);

    // Final iwpp time
    long st6 = Util::ClockGetTime();

    std::cout << "[PROFILING][IWPP] iterations: " << it << std::endl;
    std::cout << "[PROFILING][IWPP_COMP] " << (st1-st0) << std::endl;
    std::cout << "[PROFILING][IWPP_EXEC] " << (st6-st1) << std::endl;
    std::cout << "[PROFILING][IWPP_FULL] " << (st6-st0) << std::endl;

    return 0;
}

#ifdef WITH_CUDA
template <typename T>
int loopedIwppReconGPU(IwppExec exOpt, cv::cuda::GpuMat& cvDevI, 
    cv::cuda::GpuMat& cvDevJ, cv::Mat& cvHostOut) {

    Halide::Target target = Halide::get_host_target();
    target.set_feature(Halide::Target::CUDA);

    Halide::Buffer<T> II = gpuMat2buf<T>(cvDevI, target, "II");
    Halide::Buffer<T> JJ = gpuMat2buf<T>(cvDevJ, target, "JJ");

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

    Halide::RVar ryi("ryi"), ryo("ryo");
    
    // int sFactor = h/56; // i.e., bridges 28 cores times 2 (for load imbalance)

    // if (exOpt == CPU) {
    //     // Schedules Raster
    //     rasterx.update(0).split(prop.y, ryo, ryi, sFactor);
    //     rasterx.update(0).allow_race_conditions(); // for parallel (ryo)
    //     rasterx.update(0).parallel(ryo);
    //     rasterx.compile_jit();

    //     // Schedules Anti-Raster
    //     arasterx.update(0).split(prop.y, ryo, ryi, sFactor);
    //     arasterx.update(0).allow_race_conditions(); // for parallel (ryo)
    //     arasterx.update(0).parallel(ryo);
    //     arasterx.compile_jit();
    // } else if (exOpt == GPU) {
    //     #ifdef WITH_CUDA
        
    //     #else
    //     std::cout << "[HalideIwpp] Attempted to schedule for GPU without "
    //         << "CUDA support." << std::endl;
    //     exit(-1);
    //     #endif
    // }

    // Scheduling variables
    Halide::RVar b, t;
    int minThreadSize = 32; // min 32 scanlines per thread

    rasterx.gpu_blocks(y).gpu_threads(x);
    rasterx.update(0).allow_race_conditions(); // for parallel (ryo)
    rasterx.update(0).split(prop.y, ryo, ryi, minThreadSize);
    rasterx.update(0).split(ryo, b, t, 16);
    rasterx.update(0).gpu_blocks(b).gpu_threads(t);
    rasterx.compile_jit(target);

    // Schedules Anti-Raster
    arasterx.gpu_blocks(y).gpu_threads(x);
    arasterx.update(0).allow_race_conditions(); 
    arasterx.update(0).split(prop.y, ryo, ryi, minThreadSize);
    arasterx.update(0).split(ryo, b, t, 16);
    arasterx.update(0).gpu_blocks(b).gpu_threads(t);
    arasterx.compile_jit(target);

    rasterx.compile_to_lowered_stmt("rasterx.html", {}, Halide::HTML, target);

    // Halide compilation time
    st1 = Util::ClockGetTime();

    // Sum structures
    unsigned long oldSum = 0;
    unsigned long newSum = 0;
    // Halide::Func lsum = halSum(JJ);
    // long* dLineSum = new long[w];
    // Halide::Buffer<long> hLineSum = Halide::Buffer<long>(dLineSum, w, "hLineSum");
    // cv::cuda::GpuMat cvDevLineSum(cv::Size(1,w), CV_32S, cv::Scalar(0));
    // Halide::Buffer<long> hLineSum = gpuMat2buf(cvDevLineSum, target, "hDevLineSum");

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
        // lsum.realize(hLineSum);

        // st4 = Util::ClockGetTime();
        // newSum = 0;
        // for (int i=0; i<w; i++)
        //     newSum += dLineSum[i];
        newSum = cv::cuda::sum(cvDevJ)[0];

        // Full iteration time
        st5 = Util::ClockGetTime();
        
        //#ifdef IT_DEBUG
        cout << "[PROFILING] it: " << it << ", sum = " << newSum << std::endl;
        cout << "[PROFILING][IWPP_PROP_TIME] " << (st3-st2) << endl;
        cout << "[PROFILING][IWPP_SUM_TIME] " << (st5-st3) << std::endl; 
        // cout << "[PROFILING][IWPP_SUM_TIME] h=" << (st4-st3) 
        //      << ", s=" << (st5-st4) << endl;
        cout << "[PROFILING][IWPP_FULL_IT_TIME] " << (st5-st2) << std::endl;
        //#endif

    } while(newSum != oldSum);

    // delete[] dLineSum;

    // Copy result to output
    cvDevJ.download(cvHostOut);
    // hOut.copy_from(JJ); // bad

    // Final iwpp time
    long st6 = Util::ClockGetTime();

    std::cout << "[PROFILING][IWPP] iterations: " << it << std::endl;
    std::cout << "[PROFILING][IWPP_COMP] " << (st1-st0) << std::endl;
    std::cout << "[PROFILING][IWPP_EXEC] " << (st6-st1) << std::endl;
    std::cout << "[PROFILING][IWPP_FULL] " << (st6-st0) << std::endl;

    return 0;
}
template int loopedIwppReconGPU<unsigned char>(IwppExec exOpt, 
    cv::cuda::GpuMat& cvDevI, cv::cuda::GpuMat& cvDevJ, cv::Mat& cvHostOut);
#endif // WITH_CUDA


template int loopedIwppRecon(IwppExec exOpt, Halide::Buffer<unsigned char>& II, 
    Halide::Buffer<unsigned char>& JJ, Halide::Buffer<unsigned char>& hOut,
    cv::cuda::GpuMat& cvDevJ, cv::Mat& cvHostOut);

#include "misc.h"

RegionTemplate* newRT(std::string name, cv::Mat* data) {
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
Halide::Buffer<T> mat2buf(cv::Mat* m, std::string name) {
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

// // This manual setup is required since GPU data may have a different
// // memory displacement than on host memory. Tldr, need to fix
// // the stride of Halide::Buffer for data already on GPU.
// template <typename T>
// Halide::Buffer<T> gpuMat2buf(cv::cuda::GpuMat& m, Halide::Target& t, 
//         std::string name) {

//     //int extents[] = {m.cols, m.rows};
//     //int strides[] = {1, ((int)m.step)/((int)sizeof(T))};
//     //int mins[] = {0, 0};
//     buffer_t devB = {0, NULL, {m.cols, m.rows}, 
//                      {1, ((int)m.step)/((int)sizeof(T))}, 
//                      {0, 0}, sizeof(T)};

    
//     Halide::Buffer<T> hDev = name.empty() ? 
//         Halide::Buffer<T>(devB) : Halide::Buffer<T>(devB, name);
//     hDev.device_wrap_native(Halide::DeviceAPI::CUDA, (intptr_t)m.data, t);

//     return hDev;
// }
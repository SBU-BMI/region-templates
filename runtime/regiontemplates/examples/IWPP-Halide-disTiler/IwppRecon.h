#ifndef IWPP_RECON_H
#define IWPP_RECON_H

#include "Halide.h"
#include "cv.hpp"

/* Morphological grayscale reconstruction
 * Parallel standard version implemented
 * Ref: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.92.1083&rep=rep1&type=pdf
 * Input images must have black (0) as its background and white (255) 
 * as foreground.
 * Implementation performs vertical and horizontal scans separately as a way
 * to be a stepping stone for the raster/anti-raster serial implementation.
 */
class IwppParallelRecon {
private:
    Halide::ImageParam I;
    Halide::ImageParam JJ;
    Halide::Param<int32_t> w;
    Halide::Param<int32_t> h;
    Halide::Func iwppFunc;

public:
    // Constructor which creates and compiles JIT the recon halide pipeline
    IwppParallelRecon();
    // Realizes the pipeline, writing the output J on the same input place cvJ
    // after the whole pipeline execution. The pipeline is not recompiled
    void realize(cv::Mat* cvI, cv::Mat* cvJ);
};

#endif // IWPP_RECON_H

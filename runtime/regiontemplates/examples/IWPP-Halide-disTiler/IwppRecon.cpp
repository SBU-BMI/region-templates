#include "IwppRecon.h"

IwppParallelRecon::IwppParallelRecon() : I(Halide::type_of<uint8_t>(), 2), 
        JJ(Halide::type_of<uint8_t>(), 2) {

    Halide::Func rasterx, rastery, arasterx, arastery;
    Halide::RDom se(-1,3,-1,3);
    Halide::Var x, y;

    // Clamping input
    Halide::Func J = Halide::BoundaryConditions::repeat_edge(JJ);
    
    // Clamping rasterx for rastery input
    Halide::Expr xc = clamp(x, 0, w-1);
    Halide::Expr yc = clamp(y, 0, h-1);
    Halide::Func rasterxc;

    // Functions for vertical and horizontal propagation
    rasterx(x,y) = min(I(x,y), maximum(J(x+se.x, y)));
    // arasterx(w-x,y) = min(I(w-x,y), maximum(rasterx(w-x+se.x, y)));
    rasterxc(x,y) = rasterx(xc, yc);
    rastery(x,y) = min(I(x,y), maximum(rasterxc(x, y+se.y)));
    // arastery(x,h-y) = min(I(x,h-y), maximum(rastery(x, h-y+se.y)));

    // Schedule
    rasterx.compute_root();
    Halide::Var xi, xo;
    rastery.reorder(y,x).serial(y);
    rastery.split(x, xo, xi, 16);
    rastery.vectorize(xi).parallel(xo);

    iwppFunc = rastery;
    iwppFunc.compile_jit();
}

void IwppParallelRecon::realize(cv::Mat* cvI, cv::Mat* cvJ) {
    int32_t cols = cvI->cols;
    int32_t rows = cvI->rows;
    
    // cout << "configuring" << endl;
    w.set(cols);
    h.set(rows);
    I.set(Halide::Buffer<uint8_t>(cvI->data, cols, rows));
    JJ.set(Halide::Buffer<uint8_t>(cvJ->data, cols, rows));

    Halide::Buffer<uint8_t> hOut(cvJ->data, cols, rows);

    // cout << "realizing" << endl;
    iwppFunc.realize(hOut);
}

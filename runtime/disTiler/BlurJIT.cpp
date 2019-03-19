#include "BlurJIT.h"

using namespace std;

BlurJIT::BlurJIT(cv::Mat &input, cv::Mat &output) {
    // this->input = input;

    // name halide functions
    hf_output = Halide::Func("output(blur-y)");
    blurx = Halide::Func("blur-x");
    x = Halide::Var("x");
    y = Halide::Var("y");

    this->rows = input.rows;
    this->cols = input.cols;

    hb_input = Halide::Buffer<uint8_t>(input.data, this->cols, this->rows);
    hb_input.set_name("input");

    hb_output = Halide::Buffer<uint8_t>(output.data, this->cols, this->rows);
    hb_output.set_name("output");    

    clamped(x,y) = Halide::cast<uint16_t>(Halide::BoundaryConditions::repeat_edge(hb_input)(x,y));

    blurx(x,y) = (clamped(x-1,y) + clamped(x,y) + clamped(x+1,y))/3;
    hf_output(x,y) = Halide::cast<uint8_t>((blurx(x,y-1) + blurx(x,y) + blurx(x,y+1))/3);
}

ret_t BlurJIT::sched() {

}

ret_t BlurJIT::run() {
    hf_output.realize(hb_output);
}

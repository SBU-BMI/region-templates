#include "Halide.h"

class BlurAOTGen : public Halide::Generator<BlurAOTGen> {
public:
    // inputs and outputs of the pipeline
    Input<Halide::Buffer<uint8_t>> input{"input", 2};
    Output<Halide::Buffer<uint8_t>> output{"output", 2};

    // internal variables
    Halide::Var x, y;


    void generate() {
        // pipeline stages/funcs
        Halide::Func blurx;
        Halide::Func clamped;
        
        // name halide variables
        x = Halide::Var("x");
        y = Halide::Var("y");

        clamped(x,y) = Halide::cast<uint16_t>(Halide::BoundaryConditions::repeat_edge(input)(x,y));

        blurx(x,y) = (clamped(x-1,y) + clamped(x,y) + clamped(x+1,y))/3;
        output(x,y) = Halide::cast<uint8_t>((blurx(x,y-1) + blurx(x,y) + blurx(x,y+1))/3);
        // output(x,y) = input(x,y);
    }
};
HALIDE_REGISTER_GENERATOR(BlurAOTGen, blurAOT)

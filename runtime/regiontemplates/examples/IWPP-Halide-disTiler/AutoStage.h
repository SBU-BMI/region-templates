#ifndef AUTO_STAGE_H
#define AUTO_STAGE_H

#include <stdint.h>
#include <string>
#include <vector>

#include "Halide.h"

#include "SysEnv.h"
#include "RegionTemplate.h"
#include "DenseDataRegion2D.h"
#include "RTPipelineComponentBase.h"

#define DATA_T uint8_t

namespace RTF {

class AutoStage; // Forward def for ASInputs types

template <typename T_PARAM = uint8_t> // Only for Param's
class ASInputs {
public:
    enum Types {
        // Halide::Expr, // cannot be pure halide
        // AutoStage, // Not yet implemented
        RT,
        Param // const parameter value (e.g., int, float, ...)
    };

    ASInputs(std::string rt_name) : rt_name(rt_name) {}
    ASInputs(T_PARAM param) : param(param), type(Param) {}

    RegionTemplate* getRT(RTPipelineComponentBase* pcb) {
        return pcb->getRegionTemplateInstance(this->rt_name);
    }
    T_PARAM getParam() {return param;}

    Types getType() {return type;}
private:
    Types type;
    T_PARAM param;
    std::string rt_name;
};

enum Target_t {
    CPU,
    GPU
};

template <typename T = uint8_t>
class HalImgParamOrParam {
    Halide::Param<T> param;
    Halide::ImageParam imgParam;

public:
    HalImgParamOrParam(Halide::Param<T> p) : param(p) {}
    HalImgParamOrParam(Halide::ImageParam ip) : imgParam(ip) {}

    Halide::ImageParam getImParam() {return imgParam;}
    Halide::Param<T> getParam() {return param;}
};

struct HalGen {
    virtual void generate(std::map<Target_t, Halide::Func>& schedules,
        std::vector<HalImgParamOrParam<>>& params) = 0;
};

class AutoStage : public RTPipelineComponentBase {
    SysEnv sysEnv;
    std::vector<ASInputs<>> ios;
    std::vector<int> out_shape; // Rows at 0, cols at 1
    Target_t this_target;
    HalGen* halGenFun;

public:
    AutoStage(const std::vector<int>& out_shape,
              const std::vector<ASInputs<>>& ios,
              Target_t this_target,
              HalGen* halGenFun);
    ~AutoStage();

    // First implementation only has one stage
    void execute(int argc, char** argv);

    int run();
};

} // namespace RTF

#endif // AUTO_STAGE_H

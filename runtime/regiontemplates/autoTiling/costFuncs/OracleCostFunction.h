#ifndef ORACLE_COST_FUNCTION_H_
#define ORACLE_COST_FUNCTION_H_

#include <string>

#include "cv.hpp"
#include "Halide.h"

#include "ExecEngineConstants.h"
#include "RegionTemplate.h"

#include "CostFunction.h"

class OracleCostFunction : public CostFunction {
public:
    OracleCostFunction();

    // T cost(cv::Mat img);
    int64_t cost(cv::Mat img) const;
    cv::Mat costImg(cv::Mat img) const;
};

#endif

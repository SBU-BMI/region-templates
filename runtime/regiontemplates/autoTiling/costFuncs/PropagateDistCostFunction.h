#ifndef PROPAGATE_DIST_COST_FUNCTION_H_
#define PROPAGATE_DIST_COST_FUNCTION_H_

#include <string>

#include <opencv/cv.hpp>

#include "CostFunction.h"
#include "BGMasker.h"
#include "ColorThresholdBGMasker.h"

#define CV_MAX_PIX_VAL 255
#define CV_THR_BIN 0
#define CV_THR_BIN_INV 1

class PropagateDistCostFunction : public CostFunction {
private:
    int e1, d1;
    int e2, d2;
    
public:
    // PropagateDistCostFunction(int e1=10, int d1=10, int e2=15, int d2=10);
    PropagateDistCostFunction(int e1=10, int e2=15, int d2=10);

    // T cost(cv::Mat img);
    int64_t cost(cv::Mat img) const;
    cv::Mat costImg(cv::Mat img) const;
};

#endif

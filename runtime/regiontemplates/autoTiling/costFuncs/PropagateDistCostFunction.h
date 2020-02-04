#ifndef PROPAGATE_DIST_COST_FUNCTION_H_
#define PROPAGATE_DIST_COST_FUNCTION_H_

#include <string>

#include <opencv/cv.hpp>

#include "CostFunction.h"
#include "BGMasker.h"
#include "ThresholdBGMasker.h"
#include "ColorThresholdBGMasker.h"

#define CV_MAX_PIX_VAL 255
#define CV_THR_BIN 0
#define CV_THR_BIN_INV 1

class PropagateDistCostFunction : public CostFunction {
private:
    BGMasker* bgm;
    
public:
    PropagateDistCostFunction(int bgThr, int dilate, int erode);
    PropagateDistCostFunction(BGMasker* bgm);

    // T cost(cv::Mat img);
    int64_t cost(cv::Mat img) const;
    cv::Mat costImg(cv::Mat img) const;
};

#endif

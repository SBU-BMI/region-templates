#ifndef THRESHOLD_BG_COST_FUNCTION_H_
#define THRESHOLD_BG_COST_FUNCTION_H_

#include <string>

#include <opencv/cv.hpp>

#include "CostFunction.h"
#include "ThresholdBGMasker.h"

#define CV_MAX_PIX_VAL 255
#define CV_THR_BIN 0
#define CV_THR_BIN_INV 1

// template <typename T>
// class ThresholdBGCostFunction : public CostFunction<T> {
class ThresholdBGCostFunction : public CostFunction {
private:
    ThresholdBGMasker* bgm;
    
public:
    ThresholdBGCostFunction(int bgThr, int dilate, int erode);
    ThresholdBGCostFunction(ThresholdBGMasker* bgm);

    // T cost(cv::Mat img);
    int64_t cost(cv::Mat img);
    cv::Mat costImg(cv::Mat img);
};

#endif

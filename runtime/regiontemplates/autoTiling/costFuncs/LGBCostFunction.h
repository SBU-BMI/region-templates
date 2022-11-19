#ifndef LGB_COST_FUNCTION_H_
#define LGB_COST_FUNCTION_H_

#include <string>

#include <opencv/cv.hpp>

#include "CostFunction.h"
#include "ThresholdBGMasker.h"

// #define CV_MAX_PIX_VAL 255
// #define CV_THR_BIN 0
// #define CV_THR_BIN_INV 1

class LGBCostFunction : public CostFunction {
  private:
    ThresholdBGMasker *bgm;

  public:
    LGBCostFunction(int bgThr, int dilate, int erode);
    LGBCostFunction(ThresholdBGMasker *bgm);

    // T cost(cv::Mat img);
    double  cost(const cv::Mat &img) const;
    cv::Mat costImg(const cv::Mat &img) const;
};

#endif

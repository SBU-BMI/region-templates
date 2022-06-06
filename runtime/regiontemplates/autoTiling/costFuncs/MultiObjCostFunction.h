#ifndef MULTI_OBJ_COST_FUNCTION_H_
#define MULTI_OBJ_COST_FUNCTION_H_

#include <string>

#include <opencv/cv.hpp>

#include "BGMasker.h"
#include "CostFunction.h"
#include "ThresholdBGMasker.h"
// #include "ColorThresholdBGMasker.h"

#define CV_MAX_PIX_VAL 255
#define CV_THR_BIN 0
#define CV_THR_BIN_INV 1

class MultiObjCostFunction : public CostFunction {
  private:
    float execBias;
    float readBias;

  public:
    BGMasker *bgm;
    MultiObjCostFunction(int bgThr, int dilate, int erode, float execBias,
                         float readBias);
    MultiObjCostFunction(BGMasker *bgm, float execBias, float readBias);

    double  cost(const cv::Mat &img) const;
    cv::Mat costImg(const cv::Mat &img) const;
};

#endif

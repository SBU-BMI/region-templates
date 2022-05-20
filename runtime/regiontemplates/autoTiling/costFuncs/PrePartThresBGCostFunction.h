#ifndef PRE_PART_THRES_BG_COST_FUNCTION_H_
#define PRE_PART_THRES_BG_COST_FUNCTION_H_

#include <string>

#include <opencv/cv.hpp>

#include "CostFunction.h"
#include "ThresholdBGMasker.h"

#define CV_MAX_PIX_VAL 255
#define CV_THR_BIN 0
#define CV_THR_BIN_INV 1

class PrePartThresBGCostFunction : public CostFunction {
  private:
    ThresholdBGMasker *bgm;

    cv::Mat costArray;
    int     xResolution; // in number of pixels
    int     yResolution; // in number of pixels

  public:
    PrePartThresBGCostFunction(int bgThr, int dilate, int erode,
                               std::string origImgPath, int xRes, int yRes);
    PrePartThresBGCostFunction(ThresholdBGMasker *bgm, std::string origImgPath,
                               int xRes, int yRes);

    // T cost(cv::Mat img);
    double  cost(cv::Mat img) const;
    cv::Mat costImg(cv::Mat img) const;
    double  cost(cv::Mat img, cv::Rect_<int64_t> tile) const;
    double  cost(cv::Mat img, int64_t yi, int64_t yo, int64_t xi,
                 int64_t xo) const;
};

#endif

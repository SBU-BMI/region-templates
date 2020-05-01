#ifndef AREA_COST_FUNCTION_H_
#define AREA_COST_FUNCTION_H_

#include <opencv/cv.hpp>

#include "CostFunction.h"

class AreaCostFunction : public CostFunction {
public:
    AreaCostFunction() {};

    double cost(cv::Mat img) const {return img.rows*img.cols;};
    cv::Mat costImg(cv::Mat img) const {return img;};
};

#endif

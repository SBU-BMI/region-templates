#ifndef COST_FUNCTION_H_
#define COST_FUNCTION_H_

#include <iostream>
#include <opencv/cv.hpp>

// template <typename T>
class CostFunction {
public:
    CostFunction() {};

    // virtual T cost(cv::Mat img) = 0;
    virtual int64_t cost(cv::Mat img) = 0;
    int64_t cost(cv::Mat img, cv::Rect_<int64_t> tile);
    int64_t cost(cv::Mat img, int64_t yi, int64_t yo, int64_t xi, int64_t xo);

    virtual cv::Mat costImg(cv::Mat img) = 0;
    cv::Mat costImg(cv::Mat img, cv::Rect_<int64_t> tile);
    cv::Mat costImg(cv::Mat img, int64_t yi, int64_t yo, int64_t xi, int64_t xo);
};

#endif

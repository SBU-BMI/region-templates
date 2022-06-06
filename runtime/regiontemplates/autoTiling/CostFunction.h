#ifndef COST_FUNCTION_H_
#define COST_FUNCTION_H_

#include <iostream>
#include <opencv/cv.hpp>

// template <typename T>
class CostFunction {
  public:
    CostFunction(){};

    // virtual T cost(cv::Mat img) = 0;
    virtual double cost(const cv::Mat &img) const = 0;
    virtual double cost(const cv::Mat &img, cv::Rect_<int64_t> tile) const;
    virtual double cost(const cv::Mat &img, int64_t yi, int64_t yo, int64_t xi,
                        int64_t xo) const;

    virtual cv::Mat costImg(const cv::Mat &img) const = 0;
    virtual cv::Mat costImg(const cv::Mat &img, cv::Rect_<int64_t> tile) const;
    virtual cv::Mat costImg(const cv::Mat &img, int64_t yi, int64_t yo,
                            int64_t xi, int64_t xo) const;
};

#endif
